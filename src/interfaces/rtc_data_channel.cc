/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_data_channel.h"

#include <utility>

#include <webrtc/api/data_channel_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/rtc_base/copy_on_write_buffer.h>

#include "src/converters/absl.h"
#include "src/enums/node_webrtc/binary_type.h"
#include "src/enums/webrtc/data_state.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/node/envelope.h"
#include "src/node/error_factory.h"
#include "src/utilities/log.h"
#include "src/utilities/napi_ref_ptr.h"
#include "src/utilities/task.h"

namespace node_webrtc {

    RTCDataChannel::RTCDataChannel(const Napi::CallbackInfo& info) :
        Proxy<RTCDataChannel, webrtc::DataChannelInterface>(info) 
    {
        Construct(info);

        // In previous versions we had some complexity around connecting an observer early and replaying events.
        // In M150 this is not necessary, the data is already queued until an observer is registered, and state() is a blocking
        // call to the network thread. So just register the observer, and if we haven't sent an open state event yet after
        // that (and we're already open), just synthesize an OnStateChange() event.
        _handle->RegisterObserver(this);
        if (!hasOpened && _handle->state() == webrtc::DataChannelInterface::kOpen)
            OnStateChange();
    }

    void RTCDataChannel::CleanupInternals() {
        if (_handle == nullptr)
            return;

        UnregisterProxy();
        _handle->UnregisterObserver();
        _cached_id = _handle->id();
        _cached_label = _handle->label();
        _cached_max_packet_life_time = _handle->maxPacketLifeTime().value_or(0); // TODO(liam): optional handling?
        _cached_max_retransmits = _handle->maxRetransmitsOpt().value_or(0); // TODO(liam): optional handling?
        _cached_negotiated = _handle->negotiated();
        _cached_ordered = _handle->ordered();
        _cached_protocol = _handle->protocol();
        _cached_buffered_amount = _handle->buffered_amount();
        _handle = nullptr;
    }

    void RTCDataChannel::OnPeerConnectionClosed() {
        if (_handle != nullptr) {
            Stop();
            CleanupInternals();
        }
    }

    void RTCDataChannel::OnStateChange() {
        auto state = _handle->state();
        Log(this, "RTCDataChannel::OnStateChange(" + std::to_string(state) + ")");

        // Only send one open event, even if we encounter one later.
        if (state == webrtc::DataChannelInterface::kOpen) {
            if (hasOpened)
                return;
            hasOpened = true;
        }

        if (state == webrtc::DataChannelInterface::kClosed) {
            CleanupInternals();
            Stop();
            _handle = nullptr;
        }

        Dispatch(CreateTask([this, state]() {
            if (state == webrtc::DataChannelInterface::kClosed) {
                Event("close").Dispatch();
            } else if (state == webrtc::DataChannelInterface::kOpen) {
                Event("open").Dispatch();
            }
        }));
    }

    void RTCDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
        Dispatch(CreateTask([this, buffer]() {
            RTCDataChannel::HandleMessage(*this, buffer);
        }));
    }

    void RTCDataChannel::HandleMessage(RTCDataChannel& channel, const webrtc::DataBuffer& buffer) {
        bool binary = buffer.binary;
        size_t size = buffer.size();

        auto env = channel.Env();
        Napi::HandleScope scope(env);
        Napi::Value value;
        if (binary) {
            char* message = new char[size];
            memcpy(reinterpret_cast<void*>(message), reinterpret_cast<const void*>(buffer.data.data()), size);
            auto array = Napi::ArrayBuffer::New(env, message, size, [](Napi::Env, void* buffer) {
                delete[] static_cast<char*>(buffer);
            });
            value = array; // NOLINT
        } else {
            auto str = Napi::String::New(env, reinterpret_cast<const char*>(buffer.data.data()), size); // NOLINT
            value = str;
        }
        auto object = Napi::Object::New(env);
        object.Set("type", "message");
        object.Set("data", value);
        channel.MakeCallback("dispatchEvent", {object});
    }

    Napi::Value RTCDataChannel::Send(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        if (_handle != nullptr) {
            if (_handle->state() != webrtc::DataChannelInterface::DataState::kOpen) {
                Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "RTCDataChannel.readyState is not 'open'")).ThrowAsJavaScriptException();
                return env.Undefined();
            }
            if (info[0].IsString()) {
                auto str = info[0].ToString();
                auto data = str.Utf8Value();

                webrtc::DataBuffer buffer(data);
                _handle->Send(buffer);
            } else {
                Napi::ArrayBuffer arraybuffer;
                size_t byte_offset = 0;
                size_t byte_length = 0;

                if (info[0].IsTypedArray()) {
                    auto typedArray = info[0].As<Napi::TypedArray>();
                    arraybuffer = typedArray.ArrayBuffer();
                    byte_offset = typedArray.ByteOffset();
                    byte_length = typedArray.ByteLength();
                } else if (info[0].IsDataView()) {
                    auto dataView = info[0].As<Napi::DataView>();
                    arraybuffer = dataView.ArrayBuffer();
                    byte_offset = dataView.ByteOffset();
                    byte_length = dataView.ByteLength();
                } else if (info[0].IsArrayBuffer()) {
                    arraybuffer = info[0].As<Napi::ArrayBuffer>();
                    byte_length = arraybuffer.ByteLength();
                } else {
                    Napi::TypeError::New(env, "Expected a Blob or ArrayBuffer").ThrowAsJavaScriptException();
                    return env.Undefined();
                }

                char* content = static_cast<char*>(arraybuffer.Data());
                webrtc::CopyOnWriteBuffer buffer(content + byte_offset, byte_length);

                webrtc::DataBuffer data_buffer(buffer, true);
                _handle->Send(data_buffer);
            }
        } else {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "RTCDataChannel.readyState is not 'open'")).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        return env.Undefined();
    }

    Napi::Value RTCDataChannel::Close(const Napi::CallbackInfo& info) {
        if (_handle != nullptr) {
            _handle->Close();
        }
        return info.Env().Undefined();
    }

    Napi::Value RTCDataChannel::GetBufferedAmount(const Napi::CallbackInfo& info) {
        uint64_t buffered_amount = _handle != nullptr
            ? _handle->buffered_amount()
            : _cached_buffered_amount;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), buffered_amount, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetId(const Napi::CallbackInfo& info) {
        auto id = _handle
            ? _handle->id()
            : _cached_id;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), id, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetLabel(const Napi::CallbackInfo& info) {
        auto label = _handle != nullptr
            ? _handle->label()
            : _cached_label;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), label, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetMaxPacketLifeTime(const Napi::CallbackInfo& info) {
        auto max_packet_life_time = _handle
            ? _handle->maxPacketLifeTime()
            : _cached_max_packet_life_time;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), max_packet_life_time, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetMaxRetransmits(const Napi::CallbackInfo& info) {
        auto max_retransmits = _handle
            ? _handle->maxRetransmitsOpt()
            : _cached_max_retransmits;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), max_retransmits, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetNegotiated(const Napi::CallbackInfo& info) {
        auto negotiated = _handle
            ? _handle->negotiated()
            : _cached_negotiated;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), negotiated, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetOrdered(const Napi::CallbackInfo& info) {
        auto ordered = _handle
            ? _handle->ordered()
            : _cached_ordered;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), ordered, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetPriority(const Napi::CallbackInfo& info) {
        std::string priority = "high";
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), priority, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetProtocol(const Napi::CallbackInfo& info) {
        auto protocol = _handle
            ? _handle->protocol()
            : _cached_protocol;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), protocol, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetReadyState(const Napi::CallbackInfo& info) {
        auto state = _handle
            ? _handle->state()
            : webrtc::DataChannelInterface::kClosed;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), state, result, Napi::Value)
        return result;
    }

    Napi::Value RTCDataChannel::GetBinaryType(const Napi::CallbackInfo& info) {
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _binaryType, result, Napi::Value)
        return result;
    }

    void RTCDataChannel::SetBinaryType(const Napi::CallbackInfo& info, const Napi::Value& value) {
        auto maybeBinaryType = From<BinaryType>(value);
        if (maybeBinaryType.IsInvalid()) {
            Throw<Napi::TypeError>(info.Env(), maybeBinaryType.ToErrors()[0]);
            return;
        }

        if (maybeBinaryType.UnsafeFromValid() == BinaryType::kBlob) {
            // TODO(liam): Support blobs
            Throw<Napi::TypeError>(info.Env(), "binaryType 'blob' is not yet supported");
            return;
        }

        _binaryType = maybeBinaryType.UnsafeFromValid();
    }

    void RTCDataChannel::Init(Napi::Env env, Napi::Object exports) {
        auto func = DefineClass(env,
            "RTCDataChannel",
            {
                InstanceAccessor("bufferedAmount", &RTCDataChannel::GetBufferedAmount, nullptr),
                InstanceAccessor("id", &RTCDataChannel::GetId, nullptr),
                InstanceAccessor("label", &RTCDataChannel::GetLabel, nullptr),
                InstanceAccessor("maxPacketLifeTime", &RTCDataChannel::GetMaxPacketLifeTime, nullptr),
                InstanceAccessor("maxRetransmits", &RTCDataChannel::GetMaxRetransmits, nullptr),
                InstanceAccessor("negotiated", &RTCDataChannel::GetNegotiated, nullptr),
                InstanceAccessor("ordered", &RTCDataChannel::GetOrdered, nullptr),
                InstanceAccessor("priority", &RTCDataChannel::GetPriority, nullptr),
                InstanceAccessor("protocol", &RTCDataChannel::GetProtocol, nullptr),
                InstanceAccessor("binaryType", &RTCDataChannel::GetBinaryType, &RTCDataChannel::SetBinaryType),
                InstanceAccessor("readyState", &RTCDataChannel::GetReadyState, nullptr),
                InstanceMethod("close", &RTCDataChannel::Close),
                InstanceMethod("send", &RTCDataChannel::Send),
            });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("RTCDataChannel", func);
    }

} // namespace node_webrtc
