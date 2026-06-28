/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_peer_connection.h"

#include <cassert>
#include <iosfwd>
#include <memory>
#include <node-addon-api/napi.h>
#include <string>

#include <src/api/jsep.h>
#include <src/api/make_ref_counted.h>
#include <webrtc/api/environment/environment_factory.h>
#include <webrtc/api/media_types.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/rtc_error.h>
#include <webrtc/api/rtp_transceiver_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/p2p/client/basic_port_allocator.h>

#include "src/converters.h"
#include "src/converters/arguments.h"
#include "src/converters/interfaces.h"
#include "src/converters/napi.h"
#include "src/dictionaries/macros/napi.h"
#include "src/dictionaries/node_webrtc/rtc_answer_options.h"
#include "src/dictionaries/node_webrtc/rtc_offer_options.h"
#include "src/dictionaries/node_webrtc/rtc_session_description_init.h"
#include "src/dictionaries/node_webrtc/some_error.h"
#include "src/dictionaries/webrtc/data_channel_init.h"
#include "src/dictionaries/webrtc/ice_candidate_interface.h"
#include "src/dictionaries/webrtc/rtc_configuration.h"
#include "src/dictionaries/webrtc/rtc_error.h"
#include "src/dictionaries/webrtc/rtp_transceiver_init.h"
#include "src/enums/webrtc/ice_connection_state.h"
#include "src/enums/webrtc/ice_gathering_state.h"
#include "src/enums/webrtc/media_type.h"
#include "src/enums/webrtc/peer_connection_state.h"
#include "src/enums/webrtc/signaling_state.h"
#include "src/functional/either.h"
#include "src/functional/maybe.h"
#include "src/interfaces/media_stream.h"
#include "src/interfaces/media_stream_track.h"
#include "src/interfaces/rtc_data_channel.h"
#include "src/interfaces/rtc_peer_connection/create_session_description_observer.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/interfaces/rtc_peer_connection/rtc_stats_collector.h"
#include "src/interfaces/rtc_peer_connection/set_session_description_observer.h"
#include "src/interfaces/rtc_rtp_receiver.h"
#include "src/interfaces/rtc_rtp_sender.h"
#include "src/interfaces/rtc_rtp_transceiver.h"
#include "src/interfaces/rtc_sctp_transport.h"
#include "src/node/error_factory.h"
#include "src/node/events.h"
#include "src/node/promise.h"
#include "src/node/utility.h"
#include "src/utilities/log.h"

namespace {
    template <typename T>
    std::shared_ptr<T> clone_and_share(T* value) {
        return std::shared_ptr<T> {value->Clone()};
    }
}

namespace node_webrtc {

    Napi::FunctionReference& RTCPeerConnection::constructor() {
        static Napi::FunctionReference constructor;
        return constructor;
    }

    //
    // PeerConnection
    //

    RTCPeerConnection::RTCPeerConnection(const Napi::CallbackInfo& info) :
        AsyncObjectWrapWithLoop<RTCPeerConnection>("RTCPeerConnection", *this, info) {
        auto env = info.Env();

        if (!info.IsConstructCall()) {
            Napi::TypeError::New(env, "Use the new operator to construct the RTCPeerConnection.").ThrowAsJavaScriptException();
            return;
        }

        CONVERT_ARGS_OR_THROW_AND_RETURN_VOID_NAPI(info, maybeConfiguration, Maybe<ExtendedRTCConfiguration>)

        auto configuration = maybeConfiguration.FromMaybe(ExtendedRTCConfiguration());

        if (configuration.configuration.sdp_semantics == webrtc::SdpSemantics::kPlanB_DEPRECATED) {
            Throw<Napi::TypeError>(info.Env(), "Plan B is not supported");
            return;
        }

        if (!validateConfiguration(configuration.configuration)) {
            Napi::TypeError::New(info.Env(), "The given configuration is invalid.").ThrowAsJavaScriptException();
            return;
        }

        _port_range = configuration.portRange;

        // TODO(mroberts): Read `factory` (non-standard) from RTCConfiguration?
        _factory = PeerConnectionFactory::GetOrCreateDefault();
        _shouldReleaseFactory = true;

        auto result = _factory->factory()->CreatePeerConnectionOrError(
            configuration.configuration,
            webrtc::PeerConnectionDependencies {this});

        if (!result.ok()) {
            CONVERT_OR_THROW_AND_RETURN_VOID_NAPI(env, &result.error(), error, Napi::Value)
            Napi::Error(env, error).ThrowAsJavaScriptException();
            return;
        }

        _jinglePeerConnection = result.MoveValue();
        _cached_configuration = ExtendedRTCConfiguration(
            _jinglePeerConnection->GetConfiguration(),
            _port_range);
        // Now that we have an underlying PeerConnection allocated, this object must stay alive until its status becomes
        // "closed"
        // https://w3c.github.io/webrtc-pc/#garbage-collection
    }

    void RTCPeerConnection::Finalize(Napi::Env env) {
        if (_factory) {
            if (_shouldReleaseFactory) {
                PeerConnectionFactory::Release();
            }
            _factory = nullptr;
        }
    }

    void RTCPeerConnection::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState state) {
        Log(this, "RTCPeerConnection::OnSignalingChange(" + std::string {webrtc::PeerConnectionInterface::AsString(state)} + ")");
        OnNodeThread([this, state]() {
            Event("signalingstatechange").Dispatch();
            if (state == webrtc::PeerConnectionInterface::kClosed) {
                Stop();
            }
        });
    }

    void RTCPeerConnection::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState state) {
        Log(this, "RTCPeerConnection::OnIceConnectionChange(" + std::string {webrtc::PeerConnectionInterface::AsString(state)} + ")");
        OnNodeThread([this, state]() {
            Event("iceconnectionstatechange")
                .With("_state", state)
                .Dispatch();
            // Event("connectionstatechange")
            //     .With("_state", _jinglePeerConnection->peer_connection_state())
            //     .Dispatch();
        });
    }

    void RTCPeerConnection::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState state) {
        Log(this, "RTCPeerConnection::OnConnectionChange(" + std::string {webrtc::PeerConnectionInterface::AsString(state)} + ")");
        OnNodeThread([this, state]() {
            Event("connectionstatechange")
                .With("_state", state)
                .Dispatch();
        });
    }

    void RTCPeerConnection::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState state) {
        Log(this, "RTCPeerConnection::OnIceGatheringChange(" + std::string {webrtc::PeerConnectionInterface::AsString(state)} + ")");

        using IceGatheringState = webrtc::PeerConnectionInterface::IceGatheringState;
        using PeerConnectionState = webrtc::PeerConnectionInterface::PeerConnectionState;

        OnNodeThread([this, state]() {
            Event("icegatheringstatechange")
                .With("_state", state)
                .Dispatch();
        });

        // if we have completed gathering candidates, trigger a null candidate event
        auto connectionState = _jinglePeerConnection->peer_connection_state();
        if (state == IceGatheringState::kIceGatheringComplete && connectionState != PeerConnectionState::kClosed) {
            OnNodeThread([this]() {
                Event("icecandidate")
                    .With("candidate", Env().Null())
                    .With("target", Value())
                    .Dispatch();
            });
        }
    }

    void RTCPeerConnection::OnIceCandidate(const webrtc::IceCandidateInterface* ice_candidate) {
        Log(this, "RTCPeerConnection::OnIceCandidate(" + ice_candidate->ToString() + ")");
        std::string error;

        std::string sdp;
        if (!ice_candidate->ToString(&sdp)) {
            error = "Failed to print the candidate string. This is pretty weird. "
                    "File a bug on https://github.com/astronautlabs/webrtc";
            return;
        }

        webrtc::SdpParseError parseError;
        auto candidate = std::shared_ptr<webrtc::IceCandidateInterface>(webrtc::CreateIceCandidate(
            ice_candidate->sdp_mid(),
            ice_candidate->sdp_mline_index(),
            sdp,
            &parseError));
        if (!parseError.description.empty()) {
            error = parseError.description;
        } else if (!candidate) {
            error = "Failed to copy RTCIceCandidate";
        }

        if (error.empty()) {
            OnNodeThread([this, candidate, error]() {
                Event("icecandidate")
                    .With("candidate", candidate.get())
                    .With("target", Value())
                    .Dispatch();
            });
        }
    }

    void RTCPeerConnection::OnIceCandidateError(const std::string& address, int port, const std::string& url, int error_code, const std::string& error_text) {
        Log(this, "RTCPeerConnection::OnIceCandidateError()" + address + ":" + std::to_string(port) + ": " + std::to_string(error_code) + ": " + error_text);
        OnNodeThread([this, address, port, url, error_code, error_text]() {
            Event("icecandidateerror")
                .With("address", address)
                .With("errorCode", error_code)
                .With("errorText", error_text)
                .With("port", port)
                .With("url", url)
                .Dispatch();
        });
    }

    void RTCPeerConnection::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
        Log(this, "RTCPeerConnection::OnDataChannel(" + std::to_string(channel->id()) + ")");

        // Previous versions of this codebase immediately attached an observer to the new data channel, but this is
        // unnecessary in M150. The DataChannelInterface::state() is a blocking call to the network thread, and
        // data that arrives over the data channel is queued until such time as an observer is attached, so there's
        // no need to rush to attach an observer.

        OnNodeThread([this, channel]() {
            auto jsChannel = RTCDataChannel::Wrap(channel, _factory);
            _peerChannels.insert(jsChannel);
            Event("datachannel")
                .With("channel", jsChannel)
                .Dispatch();
        });
    }

    void RTCPeerConnection::OnAddStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
        Log(this, "RTCPeerConnection::OnAddStream(" + stream->id() + ")");
    }

    void RTCPeerConnection::OnTrack(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtpTransceiver) {
        Log(this, "RTCPeerConnection::OnTrack(" + rtpTransceiver->mid().value_or("<no-mid>") + ")");
        OnNodeThread([this, rtpTransceiver]() {
            if (!_factory || !_jinglePeerConnection)
                return;

            auto receiver = rtpTransceiver->receiver();
            auto wrappedTransceiver = createOrUpdateTransceiver(rtpTransceiver);

            auto mediaStreams = std::vector<MediaStream*>();
            for (auto const& stream : receiver->streams()) {
                auto* mediaStream = MediaStream::wrap()->GetOrCreate(_factory, stream);
                _streams.insert(mediaStream);
                mediaStreams.push_back(mediaStream);
            }

            Event("track")
                .With("receiver", wrappedTransceiver->getReceiver()->Value())
                .With("streams", mediaStreams)
                .With("track", wrappedTransceiver->getReceiver()->getTrack())
                .With("transceiver", wrappedTransceiver->Value())
                .Dispatch();
        });
    }

    void RTCPeerConnection::OnRemoveStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
        Log(this, "RTCPeerConnection::OnRemoveStream(" + stream->id() + ")");
    }

    void RTCPeerConnection::OnRenegotiationNeeded() {
        Log(this, "RTCPeerConnection::OnRenegotiationNeeded()");
        OnNodeThread([this]() {
            Event("negotiationneeded")
                .Dispatch();
        });
    }

    Napi::Value RTCPeerConnection::AddTrack(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::AddTrack()");
        auto env = info.Env();
        if (!_factory || !_jinglePeerConnection) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "Cannot addTrack; RTCPeerConnection is closed")).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        // Acquire arguments

        auto* mediaStreamTrack = MediaStreamTrack::Unwrap(info[0].As<Napi::Object>());
        if (!mediaStreamTrack) {
            Napi::TypeError::New(Env(), "track must be a MediaStreamTrack")
                .ThrowAsJavaScriptException();
            return Env().Undefined();
        }

        std::vector<MediaStream*> mediaStreams;
        for (uint64_t i = 1, max = info.Length(); i < max; ++i) {
            auto* stream = info[i].As<Napi::External<MediaStream>>().Data();
            if (!stream) {
                Napi::TypeError::New(Env(), "all streams must be MediaStream instances")
                    .ThrowAsJavaScriptException();
                return Env().Undefined();
            }
            mediaStreams.push_back(stream);
        }

        // Referencing: This track may be owned by an external source (such as an RTCAudioSource/RTCVideoSource, or a different RTCPeerConnection).
        // If we have *not* seen this track before, then it does not belong to us.
        // In that case, we need to reference it and add it to our _referencedTracks array so we can dereference it when the RTCPeerConnection closes.
        if (!isOwned(mediaStreamTrack)) {
            mediaStreamTrack->Ref();
            _externalTracks[mediaStreamTrack->getId()] = mediaStreamTrack;
        }

        std::vector<std::string> streamIds;
        streamIds.reserve(mediaStreams.size());
        for (auto const& stream : mediaStreams) {
            streamIds.emplace_back(stream->stream()->id());
        }

        auto result = _jinglePeerConnection->AddTrack(mediaStreamTrack->track(), streamIds);
        if (!result.ok()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(env, &result.error(), error, Napi::Value)
            Napi::Error(env, error).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        const auto& rtpSender = result.value();
        webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtpTransceiver;
        for (const auto& candidate : _jinglePeerConnection->GetTransceivers()) {
            if (candidate->sender() == rtpSender) {
                rtpTransceiver = candidate;
            }
        }

        assert(rtpTransceiver);
        createOrUpdateTransceiver(rtpTransceiver);

        return createOrUpdateSender(
            rtpSender,
            mediaStreamTrack->track()->kind() == webrtc::MediaStreamTrackInterface::kAudioKind ? "audio" : "video")
            ->Value();
    }

    Napi::Value RTCPeerConnection::AddTransceiver(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::AddTransceiver()");
        auto env = info.Env();

        if (!_factory || !_jinglePeerConnection) {
            Napi::Error::New(env, "Cannot addTransceiver; RTCPeerConnection is closed").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        if (_jinglePeerConnection->GetConfiguration().sdp_semantics != webrtc::SdpSemantics::kUnifiedPlan) {
            Napi::Error::New(env, "AddTransceiver is only available with Unified Plan SdpSemanticsAbort").ThrowAsJavaScriptException();
            return env.Undefined();
        }

        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, args, std::tuple<Either<webrtc::MediaType COMMA MediaStreamTrack*> COMMA Maybe<webrtc::RtpTransceiverInit>>)
        Either<webrtc::MediaType, MediaStreamTrack*> kindOrTrack = std::get<0>(args);
        Maybe<webrtc::RtpTransceiverInit> maybeInit = std::get<1>(args);
        webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::RtpTransceiverInterface>> result;
        if (kindOrTrack.IsLeft()) {
            if (maybeInit.IsNothing()) {
                result = _jinglePeerConnection->AddTransceiver(kindOrTrack.UnsafeFromLeft());
            } else {
                result = _jinglePeerConnection->AddTransceiver(kindOrTrack.UnsafeFromLeft(), maybeInit.UnsafeFromJust());
            }
        } else {
            auto rtcTrack = kindOrTrack.UnsafeFromRight()->track();
            auto* track = MediaStreamTrack::wrap()->GetOrCreate(_factory, rtcTrack);
            _tracks[rtcTrack->id()] = track;

            if (maybeInit.IsNothing()) {
                result = _jinglePeerConnection->AddTransceiver(rtcTrack);
            } else {
                result = _jinglePeerConnection->AddTransceiver(rtcTrack, maybeInit.UnsafeFromJust());
            }
        }

        if (!result.ok()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(env, &result.error(), error, Napi::Value)
            Napi::Error(env, error).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto rtpTransceiver = result.value();
        auto wrappedTransceiver = createOrUpdateTransceiver(rtpTransceiver);
        return wrappedTransceiver->Value();
    }

    Napi::Value RTCPeerConnection::RemoveTrack(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::RemoveTrack()");
        auto env = info.Env();
        if (!_factory || !_jinglePeerConnection) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "Cannot removeTrack; RTCPeerConnection is closed"))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, sender, RTCRtpSender*)

        if (!isOwned(sender)) {
            Napi::Error(env, ErrorFactory::CreateInvalidAccessError(env, "The sender was not created by this peer connection."))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto rtcSender = getUnderlying(sender);
        if (!rtcSender) {
            Napi::Error(env, ErrorFactory::CreateInvalidAccessError(env, "Cannot removeTrack"))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        if (!_factory || !_jinglePeerConnection->RemoveTrackOrError(rtcSender).ok()) {
            Napi::Error(env, ErrorFactory::CreateInvalidAccessError(env, "Cannot removeTrack"))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        createOrUpdateTransceiver(getUnderlying(sender->getTransceiver()));

        return env.Undefined();
    }

    Napi::Value RTCPeerConnection::CreateOffer(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::CreateOffer()");
        auto env = info.Env();
        CREATE_DEFERRED(env, deferred)

        auto maybeOptions = From<Maybe<RTCOfferOptions>>(Arguments(info)).Map([](auto maybeOptions) {
            return maybeOptions.FromMaybe(RTCOfferOptions());
        });
        if (maybeOptions.IsInvalid()) {
            Reject(deferred, SomeError(maybeOptions.ToErrors()[0]));
            return deferred.Promise();
        }

        if (!_jinglePeerConnection || _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            Reject(deferred, ErrorFactory::CreateInvalidStateError(env, "Failed to execute 'createOffer' on 'RTCPeerConnection': "
                                                                        "The RTCPeerConnection's signalingState is 'closed'."));
            return deferred.Promise();
        }

        auto* observer = new webrtc::RefCountedObject<CreateSessionDescriptionObserver>(this, deferred);
        _jinglePeerConnection->CreateOffer(observer, maybeOptions.UnsafeFromValid().options);

        return deferred.Promise();
    }

    Napi::Value RTCPeerConnection::CreateAnswer(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::CreateAnswer()");
        auto env = info.Env();
        CREATE_DEFERRED(env, deferred)

        auto maybeOptions = From<Maybe<RTCAnswerOptions>>(Arguments(info)).Map([](auto maybeOptions) {
            return maybeOptions.FromMaybe(RTCAnswerOptions());
        });
        if (maybeOptions.IsInvalid()) {
            Reject(deferred, SomeError(maybeOptions.ToErrors()[0]));
            return deferred.Promise();
        }

        if (!_jinglePeerConnection || _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            Reject(deferred, ErrorFactory::CreateInvalidStateError(env, "Failed to execute 'createAnswer' on 'RTCPeerConnection': "
                                                                        "The RTCPeerConnection's signalingState is 'closed'."));
            return deferred.Promise();
        }

        auto* observer = new webrtc::RefCountedObject<CreateSessionDescriptionObserver>(this, deferred);
        _jinglePeerConnection->CreateAnswer(observer, maybeOptions.UnsafeFromValid().options);

        return deferred.Promise();
    }

    Napi::Value RTCPeerConnection::SetLocalDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::SetLocalDescription()");
        auto env = info.Env();
        CREATE_DEFERRED(env, deferred)

        CONVERT_ARGS_OR_REJECT_AND_RETURN_NAPI(deferred, info, descriptionInit, RTCSessionDescriptionInit)
        if (descriptionInit.sdp.empty()) {
            descriptionInit.sdp = _lastSdp.sdp;
        }

        auto maybeRawDescription = From<std::shared_ptr<webrtc::SessionDescriptionInterface>>(descriptionInit);
        if (maybeRawDescription.IsInvalid()) {
            Reject(deferred, maybeRawDescription.ToErrors()[0]);
            return deferred.Promise();
        }

        auto description = maybeRawDescription.UnsafeFromValid();

        if (!_jinglePeerConnection || _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            Reject(deferred, ErrorFactory::CreateInvalidStateError(env, "Failed to execute 'setLocalDescription' on 'RTCPeerConnection': "
                                                                        "The RTCPeerConnection's signalingState is 'closed'."));
            return deferred.Promise();
        }

        _jinglePeerConnection->SetLocalDescription(
            description->Clone(),
            webrtc::make_ref_counted<SetSessionDescriptionObserver>(this, deferred));

        return deferred.Promise();
    }

    Napi::Value RTCPeerConnection::SetRemoteDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::SetRemoteDescription()");
        auto env = info.Env();
        CREATE_DEFERRED(env, deferred)

        CONVERT_ARGS_OR_REJECT_AND_RETURN_NAPI(deferred, info, description, std::shared_ptr<webrtc::SessionDescriptionInterface>)

        if (!_jinglePeerConnection || _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
            Reject(deferred, ErrorFactory::CreateInvalidStateError(env, "Failed to execute 'setRemoteDescription' on 'RTCPeerConnection': "
                                                                        "The RTCPeerConnection's signalingState is 'closed'."));
            return deferred.Promise();
        }

        auto* observer = new webrtc::RefCountedObject<SetSessionDescriptionObserver>(this, deferred);
        _jinglePeerConnection->SetRemoteDescription(
            description->Clone(),
            webrtc::make_ref_counted<SetSessionDescriptionObserver>(this, deferred));

        return deferred.Promise(); // NOLINT
    }

    Napi::Value RTCPeerConnection::AddIceCandidate(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::AddIceCandidate()");
        auto env = info.Env();
        CREATE_DEFERRED(env, deferred)

        CONVERT_ARGS_OR_REJECT_AND_RETURN_NAPI(deferred, info, candidate, std::shared_ptr<webrtc::IceCandidateInterface>)

        Dispatch(CreatePromise<RTCPeerConnection>(deferred, [this, candidate](auto deferred) {
            if (_jinglePeerConnection
                && _jinglePeerConnection->signaling_state() != webrtc::PeerConnectionInterface::SignalingState::kClosed
                && _jinglePeerConnection->AddIceCandidate(candidate.get())) {
                Resolve(deferred, this->Env().Undefined());
            } else {
                std::string error = std::string("Failed to set ICE candidate");
                if (!_jinglePeerConnection
                    || _jinglePeerConnection->signaling_state() == webrtc::PeerConnectionInterface::SignalingState::kClosed) {
                    error += "; RTCPeerConnection is closed";
                }
                error += ".";
                Reject(deferred, SomeError(error));
            }
        }));
        return deferred.Promise();
    }

    Napi::Value RTCPeerConnection::CreateDataChannel(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::CreateDataChannel()");
        auto env = info.Env();
        if (_jinglePeerConnection == nullptr) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "Failed to execute 'createDataChannel' on 'RTCPeerConnection': "
                                                                        "The RTCPeerConnection's signalingState is 'closed'."))
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, args, std::tuple<std::string COMMA Maybe<webrtc::DataChannelInit>>)

        auto label = std::get<0>(args);
        auto dataChannelInit = std::get<1>(args).FromMaybe(webrtc::DataChannelInit());

        auto result = _jinglePeerConnection->CreateDataChannelOrError(label, &dataChannelInit);
        if (!result.ok()) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, std::string {"Failed to create data channel: "} + result.error().message())).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        const webrtc::scoped_refptr<webrtc::DataChannelInterface>& data_channel_interface = result.value();
        if (!data_channel_interface) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "'createDataChannel' failed")).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto channel = RTCDataChannel::Wrap(data_channel_interface, _factory);
        _channels.insert(channel);
        return channel->Value();
    }

    Napi::Value RTCPeerConnection::GetConfiguration(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetConfiguration()");
        auto configuration = _jinglePeerConnection
            ? ExtendedRTCConfiguration(_jinglePeerConnection->GetConfiguration(), _port_range)
            : _cached_configuration;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), configuration, result, Napi::Value)
        return result;
    }

    bool RTCPeerConnection::validateConfiguration(webrtc::PeerConnectionInterface::RTCConfiguration configuration) {
        for (const auto& server : configuration.servers) {
            if (server.uri == std::string("undefined") && server.urls.size() == 0)
                return false;
            for (const auto& url : server.urls) {
                if (url == std::string("undefined") || url == std::string(""))
                    return false;
            }
        }

        return true;
    }

    Napi::Value RTCPeerConnection::SetConfiguration(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::SetConfiguration()");
        auto env = info.Env();

        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, configuration, webrtc::PeerConnectionInterface::RTCConfiguration)

        if (configuration.sdp_semantics == webrtc::SdpSemantics::kPlanB_DEPRECATED) {
            Throw<Napi::TypeError>(info.Env(), "Plan B is not supported");
            return env.Undefined();
        }

        if (!validateConfiguration(configuration)) {
            Throw<Napi::TypeError>(info.Env(), "The given configuration is invalid.");
            return env.Undefined();
        }

        if (!_jinglePeerConnection) {
            Napi::Error(env, ErrorFactory::CreateInvalidStateError(env, "RTCPeerConnection is closed")).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto rtcError = _jinglePeerConnection->SetConfiguration(configuration);
        if (!rtcError.ok()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(env, &rtcError, error, Napi::Value)
            Napi::Error(env, ErrorFactory::CreateSyntaxError(env, "Syntax error")).ThrowAsJavaScriptException();
            // Napi::Error(env, error).ThrowAsJavaScriptException();
            return env.Undefined();
        }

        return env.Undefined();
    }

    Napi::Value RTCPeerConnection::GetReceivers(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetReceivers()");
        std::vector<RTCRtpReceiver*> receivers;
        if (_jinglePeerConnection) {
            for (const auto& receiver : _jinglePeerConnection->GetReceivers()) {
                auto wrappedReceiver = createOrUpdateReceiver(receiver);
                receivers.emplace_back(wrappedReceiver);
            }
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), receivers, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::GetSenders(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetSenders()");
        std::vector<RTCRtpSender*> senders;

        if (_jinglePeerConnection) {
            for (const auto& sender : _jinglePeerConnection->GetSenders()) {
                auto track = sender->track();
                auto wrappedSender = createOrUpdateSender(
                    sender, sender->media_type() == webrtc::MediaType::AUDIO ? "audio" : "video");
                senders.emplace_back(wrappedSender);
            }
        }

        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), senders, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::GetStats(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetStats()");
        auto env = info.Env();

        CREATE_DEFERRED(env, deferred)

        if (!_jinglePeerConnection) {
            Reject(deferred, ErrorFactory::CreateError(env, "RTCPeerConnection is closed"));
            return deferred.Promise();
        }

        auto* callback = new webrtc::RefCountedObject<RTCStatsCollector>(this, deferred);
        _jinglePeerConnection->GetStats(callback);

        return deferred.Promise(); // NOLINT
    }

    Napi::Value RTCPeerConnection::GetTransceivers(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetTransceivers()");
        std::vector<RTCRtpTransceiver*> transceivers;
        if (_jinglePeerConnection
            && _jinglePeerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kUnifiedPlan) {
            for (const auto& transceiver : _jinglePeerConnection->GetTransceivers()) {
                auto wrappedTransceiver = createOrUpdateTransceiver(transceiver);
                transceivers.emplace_back(wrappedTransceiver);
            }
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), transceivers, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::UpdateIce(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::UpdateIce()");
        return info.Env().Undefined();
    }

    Napi::Value RTCPeerConnection::Close(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::Close()");
        Stop();
        if (_jinglePeerConnection) {
            _cached_configuration = ExtendedRTCConfiguration(
                _jinglePeerConnection->GetConfiguration(),
                _port_range);

            // Now that we are in a closed state, it is OK for us to be garbage collected.
            // Note however that if any MediaStreamTracks or DataChannels were created, those will
            // have Ref'ed us as well, so the PC will stick around until all MediaStreamTracks and DataChannels
            // are no longer reachable.
            // https://w3c.github.io/webrtc-pc/#garbage-collection
            //
            // We do not explicitly Unref() here because that happens in DidStop() after we receive the onsignalingstatechange event
            // putting us in state kClosed.

            _jinglePeerConnection->Close();

            // NOTE(mroberts): Perhaps another way to do this is to just register all remote MediaStreamTracks against this
            // RTCPeerConnection, not unlike what we do with RTCDataChannels.

            if (_jinglePeerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kUnifiedPlan) {
                for (auto pair : _tracks) { // Should this be _peerTracks?
                    pair.second->OnPeerConnectionClosed();
                }
            }

            for (auto channel : _channels) {
                channel->OnPeerConnectionClosed();
            }

            for (auto channel : _peerChannels) {
                channel->OnPeerConnectionClosed();
            }
        }

        _jinglePeerConnection = nullptr;

        if (_factory) {
            if (_shouldReleaseFactory) {
                PeerConnectionFactory::Release();
            }
            _factory = nullptr;
        }

        // We also need to remove our references to the underlying channels and tracks
        // so that in the event that they are unreachable in the GC they can be collected.
        // Since these objects also reference the PC, if they continue to be reachable,
        // the PC object will not be garbage collected.
        // If we don't do this step, RTCPeerConnection and its underlying channels and tracks will
        // leak memory.

        _channels.clear();
        _peerChannels.clear();
        _tracks.clear();

        // We may have picked up some tracks that we do not own. We want to keep these separate, because
        // we don't want to end their event loops when we're done, but we do need to unreference them
        // at this point.

        _externalTracks.clear();
        _transceivers.clear();
        _receivers.clear();
        _senders.clear();

        // Though the specification does not define this behavior, Jingle provides the backing objects
        // for our media streams, so we opt to keep a hard reference to them from the PC when they are
        // active. Unreference those as well so that they can be freed.
        // In the future when we are using our own MediaStream implementation (and using stream_ids() instead
        // of streams()) we'll be able to implement this closer to the spirit of the specification.

        _streams.clear();

        _sctpTransport = nullptr;

        return info.Env().Undefined();
    }

    Napi::Value RTCPeerConnection::RestartIce(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::RestartIce()");
        (void)info;
        if (_jinglePeerConnection) {
            _jinglePeerConnection->RestartIce();
        }
        return info.Env().Undefined();
    }

    Napi::Value RTCPeerConnection::GetCanTrickleIceCandidates(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetCanTrickleIceCandidates()");
        return info.Env().Null();
    }

    Napi::Value RTCPeerConnection::GetConnectionState(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetConnectionState()");
        auto env = info.Env();

        auto connectionState = _jinglePeerConnection
            ? _jinglePeerConnection->peer_connection_state()
            : webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;

        CONVERT_OR_THROW_AND_RETURN_NAPI(env, connectionState, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::GetCurrentLocalDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetCurrentLocalDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->current_local_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->current_local_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetLocalDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetLocalDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->local_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->local_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetPendingLocalDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetPendingLocalDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->pending_local_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->pending_local_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetCurrentRemoteDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetCurrentRemoteDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->current_remote_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->current_remote_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetRemoteDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetRemoteDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->remote_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->remote_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetPendingRemoteDescription(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetPendingRemoteDescription()");
        auto env = info.Env();
        Napi::Value result = env.Null();
        if (_jinglePeerConnection && _jinglePeerConnection->pending_remote_description()) {
            CONVERT_OR_THROW_AND_RETURN_NAPI(
                env,
                clone_and_share(_jinglePeerConnection->pending_remote_description()),
                description,
                Napi::Value)
            result = description;
        }
        return result;
    }

    Napi::Value RTCPeerConnection::GetSctp(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetSctp()");
        if (!_jinglePeerConnection || !_jinglePeerConnection->GetSctpTransport())
            return info.Env().Null();

        if (!_sctpTransport)
            _sctpTransport = RTCSctpTransport::wrap()->GetOrCreate(_factory, _jinglePeerConnection->GetSctpTransport());

        return _sctpTransport->Value();
    }

    Napi::Value RTCPeerConnection::GetSignalingState(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetSignalingState()");
        auto signalingState = _jinglePeerConnection
            ? _jinglePeerConnection->signaling_state()
            : webrtc::PeerConnectionInterface::SignalingState::kClosed;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), signalingState, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::GetIceConnectionState(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetIceConnectionState()");
        auto iceConnectionState = _jinglePeerConnection
            ? _jinglePeerConnection->standardized_ice_connection_state()
            : webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), iceConnectionState, result, Napi::Value)
        return result;
    }

    Napi::Value RTCPeerConnection::GetIceGatheringState(const Napi::CallbackInfo& info) {
        Log(this, "RTCPeerConnection::GetIceGatheringState()");
        auto iceGatheringState = _jinglePeerConnection
            ? _jinglePeerConnection->ice_gathering_state()
            : webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete;
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), iceGatheringState, result, Napi::Value)
        return result;
    }

    napi_ref_ptr<RTCRtpTransceiver> RTCPeerConnection::createOrUpdateTransceiver(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtpTransceiver) {
        Log(this, "RTCPeerConnection::createOrUpdateTransceiver()");
        auto id = reinterpret_cast<uintptr_t>(rtpTransceiver.get());
        std::string kind = rtpTransceiver->media_type() == webrtc::MediaType::AUDIO ? "audio" : "video";
        RTCRtpSender* sender = createOrUpdateSender(rtpTransceiver->sender(), kind);
        RTCRtpReceiver* receiver = createOrUpdateReceiver(rtpTransceiver->receiver());

        RTCRtpTransceiver* transceiver = nullptr;
        auto iter = _transceivers.find(id);

        if (iter == _transceivers.end()) {
            transceiver = RTCRtpTransceiver::Create(this, sender, receiver);
            transceiver->Ref();
            sender->setTransceiver(transceiver);
            receiver->setTransceiver(transceiver);
            transceiver->setId(id);
            _transceivers[id] = transceiver;
        } else {
            transceiver = (*iter).second;
            transceiver->updateMembers(rtpTransceiver);
        }

        return transceiver;
    }

    napi_ref_ptr<RTCRtpReceiver> RTCPeerConnection::createOrUpdateReceiver(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> rtpReceiver) {
        Log(this, "RTCPeerConnection::createOrUpdateReceiver()");
        auto iter = _receivers.find(rtpReceiver->id());
        auto* track = MediaStreamTrack::wrap()->GetOrCreate(_factory, rtpReceiver->track());

        RTCRtpReceiver* receiver = nullptr;
        if (iter == _receivers.end()) {
            receiver = RTCRtpReceiver::Create(this, track, {});
            receiver->Ref();
            // TODO(liam): Set muted state by default as in Chromium
            // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.cc;l=2320;drc=a8029174f1140518a446524b318fef5dda3fba79;bpv=1;bpt=1
            receiver->setId(rtpReceiver->id());
            _receivers[rtpReceiver->id()] = receiver;
        } else {
            receiver = (*iter).second;
        }

        if (rtpReceiver->dtls_transport()) {
            receiver->setTransport(RTCDtlsTransport::wrap()->GetOrCreate(_factory, rtpReceiver->dtls_transport()));
        } else {
            receiver->setTransport(nullptr);
        }

        return receiver;
    }

    napi_ref_ptr<RTCRtpSender> RTCPeerConnection::createOrUpdateSender(webrtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender, std::string kind) {
        Log(this, "RTCPeerConnection::createOrUpdateSender()");
        MediaStreamTrack* track = nullptr;
        if (rtpSender->track()) {
            track = getKnownTrack(rtpSender->track());
            assert(track);
        }

        auto id = rtpSender->id();
        auto iter = _senders.find(id);
        RTCRtpSender* sender = nullptr;

        if (iter == _senders.end()) {
            sender = RTCRtpSender::Create(this, kind, track, {});
            sender->Ref();
            sender->setId(id);
            _senders[id] = sender;
        } else {
            sender = (*iter).second;
            sender->setTrack(track);
        }

        if (rtpSender->dtls_transport()) {
            auto* transport = RTCDtlsTransport::wrap()->GetOrCreate(_factory, rtpSender->dtls_transport());
            sender->setTransport(transport);
        } else {
            sender->setTransport(nullptr);
        }

        return sender;
    }

    napi_ref_ptr<MediaStreamTrack> RTCPeerConnection::getTrack(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> rtpTrack) {
        Log(this, "RTCPeerConnection::getTrack()");
        auto iter = _tracks.find(rtpTrack->id());
        return iter == _tracks.end() ? nullptr : (*iter).second;
    }

    napi_ref_ptr<MediaStreamTrack> node_webrtc::RTCPeerConnection::getKnownTrack(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> rtpTrack) {
        Log(this, "RTCPeerConnection::getKnownTrack()");
        auto track = getTrack(rtpTrack);

        if (!track) {
            auto iter = _externalTracks.find(rtpTrack->id());
            track = iter == _externalTracks.end() ? nullptr : (*iter).second;
        }

        return track;
    }

    bool RTCPeerConnection::isOwned(MediaStreamTrack* track) {
        Log(this, "RTCPeerConnection::isOwned(track " + track->getId() + ")");
        return _tracks.contains(track->getId());
    }

    bool RTCPeerConnection::isOwned(RTCRtpSender* sender) {
        Log(this, "RTCPeerConnection::isOwned(sender " + sender->getId() + ")");
        return _senders.contains(sender->getId());
    }

    webrtc::scoped_refptr<webrtc::RtpSenderInterface> RTCPeerConnection::getUnderlying(RTCRtpSender* sender) {
        Log(this, "RTCPeerConnection::getUnderlying(sender " + sender->getId() + ")");
        auto rtcSenders = _jinglePeerConnection->GetSenders();
        for (auto candidate : rtcSenders) {
            if (candidate->id() == sender->getId()) {
                return candidate;
            }
        }

        return nullptr;
    }

    webrtc::scoped_refptr<webrtc::RtpReceiverInterface> RTCPeerConnection::getUnderlying(RTCRtpReceiver* receiver) {
        Log(this, "RTCPeerConnection::getUnderlying(receiver " + receiver->getId() + ")");
        auto rtcReceivers = _jinglePeerConnection->GetReceivers();
        for (auto candidate : rtcReceivers) {
            if (candidate->id() == receiver->getId()) {
                return candidate;
            }
        }

        return nullptr;
    }

    webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> RTCPeerConnection::getUnderlying(RTCRtpTransceiver* transceiver) {
        Log(this, "RTCPeerConnection::getUnderlying(transceiver " + std::to_string(transceiver->getId()) + ")");
        if (!transceiver)
            return nullptr;

        auto rtcTransceivers = _jinglePeerConnection->GetTransceivers();
        for (auto candidate : rtcTransceivers) {
            if (reinterpret_cast<uintptr_t>(candidate.get()) == transceiver->getId()) {
                return candidate;
            }
        }

        return nullptr;
    }

    void RTCPeerConnection::SaveLastSdp(const RTCSessionDescriptionInit& lastSdp) {
        Log(this, "RTCPeerConnection::SaveLastSdp(" + lastSdp.sdp + " type " + std::to_string(lastSdp.type) + ")");
        this->_lastSdp = lastSdp;
    }

    void node_webrtc::RTCPeerConnection::onSetDescriptionComplete() {
        Log(this, "RTCPeerConnection::onSetDescriptionComplete()");
        OnNodeThread([this]() {
            if (!_jinglePeerConnection)
                return;

            processStateChanges();
        });
    }

    void RTCPeerConnection::processStateChanges() {
        Log(this, "RTCPeerConnection::processStateChanges()");
        std::vector<uintptr_t> removedTransceivers;

        for (auto pair : _transceivers) {
            auto transceiver = pair.second;
            if (!getUnderlying(transceiver))
                removedTransceivers.push_back(pair.first);
        }

        for (auto key : removedTransceivers) {
            auto iter = _transceivers.find(key);
            if (iter != _transceivers.end())
                _transceivers.erase(key);
        }

        for (const auto& rtpTransceiver : _jinglePeerConnection->GetTransceivers()) {
            createOrUpdateTransceiver(rtpTransceiver);
        }
    }

    void RTCPeerConnection::Init(Napi::Env env, Napi::Object exports) {
        auto func = DefineClass(env,
            "RTCPeerConnection",
            {
                InstanceMethod("addTrack", &RTCPeerConnection::AddTrack),
                InstanceMethod("addTransceiver", &RTCPeerConnection::AddTransceiver),
                InstanceMethod("removeTrack", &RTCPeerConnection::RemoveTrack),
                InstanceMethod("createOffer", &RTCPeerConnection::CreateOffer),
                InstanceMethod("createAnswer", &RTCPeerConnection::CreateAnswer),
                InstanceMethod("setLocalDescription", &RTCPeerConnection::SetLocalDescription),
                InstanceMethod("setRemoteDescription", &RTCPeerConnection::SetRemoteDescription),
                InstanceMethod("getConfiguration", &RTCPeerConnection::GetConfiguration),
                InstanceMethod("setConfiguration", &RTCPeerConnection::SetConfiguration),
                InstanceMethod("restartIce", &RTCPeerConnection::RestartIce),
                InstanceMethod("getReceivers", &RTCPeerConnection::GetReceivers),
                InstanceMethod("getSenders", &RTCPeerConnection::GetSenders),
                InstanceMethod("getStats", &RTCPeerConnection::GetStats),
                InstanceMethod("getTransceivers", &RTCPeerConnection::GetTransceivers),
                InstanceMethod("updateIce", &RTCPeerConnection::UpdateIce),
                InstanceMethod("addIceCandidate", &RTCPeerConnection::AddIceCandidate),
                InstanceMethod("createDataChannel", &RTCPeerConnection::CreateDataChannel),
                InstanceMethod("close", &RTCPeerConnection::Close),
                InstanceAccessor("canTrickleIceCandidates", &RTCPeerConnection::GetCanTrickleIceCandidates, nullptr),
                InstanceAccessor("connectionState", &RTCPeerConnection::GetConnectionState, nullptr),
                InstanceAccessor("currentLocalDescription", &RTCPeerConnection::GetCurrentLocalDescription, nullptr),
                InstanceAccessor("localDescription", &RTCPeerConnection::GetLocalDescription, nullptr),
                InstanceAccessor("pendingLocalDescription", &RTCPeerConnection::GetPendingLocalDescription, nullptr),
                InstanceAccessor("currentRemoteDescription", &RTCPeerConnection::GetCurrentRemoteDescription, nullptr),
                InstanceAccessor("remoteDescription", &RTCPeerConnection::GetRemoteDescription, nullptr),
                InstanceAccessor("pendingRemoteDescription", &RTCPeerConnection::GetPendingRemoteDescription, nullptr),
                InstanceAccessor("sctp", &RTCPeerConnection::GetSctp, nullptr),
                InstanceAccessor("signalingState", &RTCPeerConnection::GetSignalingState, nullptr),
                InstanceAccessor("iceConnectionState", &RTCPeerConnection::GetIceConnectionState, nullptr),
                InstanceAccessor("iceGatheringState", &RTCPeerConnection::GetIceGatheringState, nullptr),
            });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("RTCPeerConnection", func);
    }

} // namespace node_webrtc
