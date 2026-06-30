/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/media_stream_track.h"
#include "src/node/envelope.h"
#include "src/utilities/log.h"
#include <src/api/media_stream_interface.h>
#include <src/api/scoped_refptr.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/rtc_base/crypto_random.h>

#include "src/converters.h"
#include "src/converters/interfaces.h"
#include "src/enums/webrtc/track_state.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"

#define Super Proxy<MediaStreamTrack, webrtc::MediaStreamTrackInterface>

namespace node_webrtc {
    MediaStreamTrack::MediaStreamTrack(const Napi::CallbackInfo& info) :
        Super(info) 
    {
        Construct(info);
        _handle->RegisterObserver(this);
    }

    void MediaStreamTrack::Finalize(Napi::Env env) {
        Super::Finalize(env);
        if (_handle)
            _handle->UnregisterObserver(this);
        _handle = nullptr;
    }

    void MediaStreamTrack::Stop() {
        Log(this, "MediaStreamTrack::Stop()");
        _handle->UnregisterObserver(this);
        _ended = true;
        _enabled = _handle->enabled();
        Super::Stop();
    }

    void MediaStreamTrack::OnChanged() {
        Log(this, "MediaStreamTrack::OnChanged()");
        // Important to dispatch onto our own thread, because we may be in the process of shutting down
        // via PeerConnection.Close(). If that is the case, stopping the track will attempt to unregister
        // the observer on the underlying MediaStreamTrack which will cause a deadlock between signalling
        // and worker threads.
        if (_handle->state() == webrtc::MediaStreamTrackInterface::TrackState::kEnded) {
            Stop();
        }
    }

    void MediaStreamTrack::OnPeerConnectionClosed() {
        Stop();
    }

    Napi::Value MediaStreamTrack::GetEnabled(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::GetEnabled()");
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _ended ? _enabled : _handle->enabled(), result, Napi::Value)
        return result;
    }

    void MediaStreamTrack::SetEnabled(const Napi::CallbackInfo& info, const Napi::Value& value) {
        Log(this, "MediaStreamTrack::SetEnabled()");
        auto maybeEnabled = From<bool>(value);
        if (maybeEnabled.IsInvalid()) {
            Napi::TypeError::New(info.Env(), maybeEnabled.ToErrors()[0]).ThrowAsJavaScriptException();
            return;
        }
        auto enabled = maybeEnabled.UnsafeFromValid();
        if (_ended) {
            _enabled = enabled;
        } else {
            _handle->set_enabled(enabled);
        }
    }

    Napi::Value MediaStreamTrack::GetId(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::GetId()");
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _handle->id(), result, Napi::Value)
        return result;
    }

    Napi::Value MediaStreamTrack::GetKind(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::GetKind()");
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _handle->kind(), result, Napi::Value)
        return result;
    }

    Napi::Value MediaStreamTrack::GetReadyState(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::GetReadyState()");
        auto state = _ended
            ? webrtc::MediaStreamTrackInterface::TrackState::kEnded
            : _handle->state();
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), state, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStreamTrack::GetMuted(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::GetMuted()");
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), false, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStreamTrack::Clone(const Napi::CallbackInfo&) {
        Log(this, "MediaStreamTrack::Clone()");
        auto label = webrtc::CreateRandomUuid();
        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> clonedTrack = nullptr;
        if (_handle->kind() == _handle->kAudioKind) {
            auto audioTrack = static_cast<webrtc::AudioTrackInterface*>(_handle.get());
            clonedTrack = _factory->factory()->CreateAudioTrack(label, audioTrack->GetSource());
        } else {
            auto videoTrack = static_cast<webrtc::VideoTrackInterface*>(_handle.get());
            clonedTrack = _factory->factory()->CreateVideoTrack(webrtc::scoped_refptr {videoTrack->GetSource()}, label);
        }

        auto clonedMediaStreamTrack = Wrap(clonedTrack, _factory);
        if (_ended) {
            clonedMediaStreamTrack->Stop();
        }
        return clonedMediaStreamTrack->Value();
    }

    Napi::Value MediaStreamTrack::JsStop(const Napi::CallbackInfo& info) {
        Log(this, "MediaStreamTrack::JsStop()");
        Stop();
        return info.Env().Undefined();
    }

    MediaStreamTrack* MediaStreamTrack::Create(
        PeerConnectionFactory* factory,
        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) {
        auto env = constructor().Env();
        Napi::HandleScope scope(env);

        auto mediaStreamTrack = Unwrap(constructor().New({factory->Value(), Napi::CreateEnvelope(env, track)}));

        return mediaStreamTrack;
    }

    void MediaStreamTrack::Init(Napi::Env env, Napi::Object exports) {
        auto func = DefineClass(env,
            "MediaStreamTrack",
            {
                InstanceAccessor("enabled", &MediaStreamTrack::GetEnabled, &MediaStreamTrack::SetEnabled),
                InstanceAccessor("id", &MediaStreamTrack::GetId, nullptr),
                InstanceAccessor("kind", &MediaStreamTrack::GetKind, nullptr),
                InstanceAccessor("readyState", &MediaStreamTrack::GetReadyState, nullptr),
                InstanceAccessor("muted", &MediaStreamTrack::GetMuted, nullptr),
                InstanceMethod("clone", &MediaStreamTrack::Clone),
                InstanceMethod("stop", &MediaStreamTrack::JsStop),
            });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("MediaStreamTrack", func);
    }

    // CONVERTER_IMPL(MediaStreamTrack*, webrtc::scoped_refptr<webrtc::AudioTrackInterface>, mediaStreamTrack) {
    //     auto track = mediaStreamTrack->track();
    //     if (track->kind() != webrtc::MediaStreamTrackInterface::kAudioKind) {
    //         return Validation<webrtc::scoped_refptr<webrtc::AudioTrackInterface>>::Invalid(
    //             "Expected an audio MediaStreamTrack");
    //     }
    //     webrtc::scoped_refptr<webrtc::AudioTrackInterface> audioTrack(static_cast<webrtc::AudioTrackInterface*>(track.get()));
    //     return Pure(audioTrack);
    // }

    // CONVERTER_IMPL(MediaStreamTrack*, webrtc::scoped_refptr<webrtc::VideoTrackInterface>, mediaStreamTrack) {
    //     auto track = mediaStreamTrack->track();
    //     if (track->kind() != webrtc::MediaStreamTrackInterface::kVideoKind) {
    //         return Validation<webrtc::scoped_refptr<webrtc::VideoTrackInterface>>::Invalid(
    //             "Expected a video MediaStreamTrack");
    //     }
    //     webrtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack(static_cast<webrtc::VideoTrackInterface*>(track.get()));
    //     return Pure(videoTrack);
    // }

    // CONVERT_INTERFACE_TO_AND_FROM_NAPI(MediaStreamTrack, "MediaStreamTrack")

    // CONVERT_VIA(Napi::Value, MediaStreamTrack*, webrtc::scoped_refptr<webrtc::AudioTrackInterface>)
    // CONVERT_VIA(Napi::Value, MediaStreamTrack*, webrtc::scoped_refptr<webrtc::VideoTrackInterface>)

} // namespace node_webrtc
