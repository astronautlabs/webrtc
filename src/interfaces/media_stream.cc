/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/media_stream.h"
#include "src/utilities/napi_ref_ptr.h"
#include "src/utilities/webrtc_utils.h"
#include <node-addon-api/napi.h>
#include <src/api/media_stream_interface.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/rtc_base/crypto_random.h>

#include "src/converters.h"
#include "src/converters/arguments.h"
#include "src/converters/napi.h"
#include "src/interfaces/media_stream_track.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"


namespace node_webrtc {
    #define Super Proxy<MediaStream, webrtc::MediaStreamInterface>

    MediaStream::MediaStream(const Napi::CallbackInfo& info):
        Proxy<MediaStream, webrtc::MediaStreamInterface>(info)
    {
        Construct(info);
    }

    void MediaStream::Construct(const Napi::CallbackInfo &info) {
        // ---------------------------------------------------------
        // new MediaStream()
        // ---------------------------------------------------------
        if (info.Length() == 0 || info[0].IsUndefined()) {
            
            _factory = PeerConnectionFactory::GetOrCreateDefault();
            _factory->Ref();
            _shouldReleaseFactory = true;
            _stream = _factory->factory()->CreateLocalMediaStream(webrtc::CreateRandomUuid());
            return;
        } 
        
        // ---------------------------------------------------------
        // new MediaStream(tracks)
        // ---------------------------------------------------------
        if (info.Length() == 1 && info[0].IsArray()) {
            auto tracks = MediaStreamTrack::UnwrapArray(info[0]);

            _factory = tracks.empty() ? PeerConnectionFactory::GetOrCreateDefault() : tracks[0]->factory();
            _shouldReleaseFactory = tracks.empty();

            _stream = _factory->factory()->CreateLocalMediaStream(webrtc::CreateRandomUuid());

            for (auto const& track : tracks) {
                if (track->track()->kind() == track->track()->kAudioKind)
                    _stream->AddTrack(Cast<webrtc::AudioTrackInterface>(track->track()));
                else
                    _stream->AddTrack(Cast<webrtc::VideoTrackInterface>(track->track()));
            }
        }

        // ---------------------------------------------------------
        // new MediaStream(localStream)
        // ---------------------------------------------------------

        if (info.Length() == 1 && MediaStream::IsInstance(info[0])) {
            auto existingStream = MediaStream::UnwrapProxy(info[0]);
            auto factory = existingStream->_factory;
            auto tracks = std::vector<napi_ref_ptr<MediaStreamTrack>>();
            for (auto const& track : existingStream->tracks()) {
                tracks.push_back(MediaStreamTrack::Wrap(track, factory));
            }
            
            _factory = factory;
            _shouldReleaseFactory = !factory && tracks.empty();
            _stream = _factory->factory()->CreateLocalMediaStream(webrtc::CreateRandomUuid());

            for (auto const& track : tracks) {
                if (track->track()->kind() == track->track()->kAudioKind)
                    _stream->AddTrack(Cast<webrtc::AudioTrackInterface>(track->track()));
                else
                    _stream->AddTrack(Cast<webrtc::VideoTrackInterface>(track->track()));
            }
        }

        // ---------------------------------------------------------
        // new MediaStream(init: { id: string })
        // ---------------------------------------------------------

        if (info.Length() == 1 && info[0].IsObject() && info[0].As<Napi::Object>().Has("id")) {
            _factory = PeerConnectionFactory::GetOrCreateDefault();
            _factory->Ref();
            _shouldReleaseFactory = true;
            _stream = _factory->factory()->CreateLocalMediaStream(
                ToOrThrow<std::string>(
                    info.Env(), info[0].As<Napi::Object>().Get("id"),
                    "Expected a string for 'id' init property"
                )
            );
        }

        // ---------------------------------------------------------
        // new MediaStream(<handle>, <factory>)
        // + exception for unsupported pattern
        // ---------------------------------------------------------

        Super::Construct(info);
    }

    void MediaStream::Finalize(Napi::Env env) {    
        _factory = nullptr;

        // TODO(liam): remove
        if (_shouldReleaseFactory)
            PeerConnectionFactory::Release();
    }

    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> MediaStream::tracks() {
        auto tracks = std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>();
        for (auto const& track : _stream->GetAudioTracks()) {
            tracks.emplace_back(track);
        }
        for (auto const& track : _stream->GetVideoTracks()) {
            tracks.emplace_back(track);
        }
        return tracks;
    }

    Napi::Value MediaStream::GetId(const Napi::CallbackInfo& info) {
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _stream->id(), result, Napi::Value)
        return result;
    }

    Napi::Value MediaStream::GetActive(const Napi::CallbackInfo& info) {
        auto active = false;
        for (auto const& track : tracks()) {
            auto mediaStreamTrack = MediaStreamTrack::Wrap(track, _factory);
            active = active || mediaStreamTrack->active();
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), active, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStream::GetAudioTracks(const Napi::CallbackInfo& info) {
        auto tracks = std::vector<napi_ref_ptr<MediaStreamTrack>>();
        for (auto const& track : _stream->GetAudioTracks()) {
            auto mediaStreamTrack = MediaStreamTrack::Wrap(track, _factory);
            tracks.push_back(mediaStreamTrack);
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), tracks, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStream::GetVideoTracks(const Napi::CallbackInfo& info) {
        auto tracks = std::vector<napi_ref_ptr<MediaStreamTrack>>();
        for (auto const& track : _stream->GetVideoTracks()) {
            auto mediaStreamTrack = MediaStreamTrack::Wrap(track, _factory);
            tracks.push_back(mediaStreamTrack);
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), tracks, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStream::GetTracks(const Napi::CallbackInfo& info) {
        auto tracks = std::vector<napi_ref_ptr<MediaStreamTrack>>();
        for (auto const& track : this->tracks()) {
            auto mediaStreamTrack = MediaStreamTrack::Wrap(track, _factory);
            tracks.push_back(mediaStreamTrack);
        }
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), tracks, result, Napi::Value)
        return result;
    }

    Napi::Value MediaStream::GetTrackById(const Napi::CallbackInfo& info) {
        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, label, std::string)
        auto audioTrack = _stream->FindAudioTrack(label);
        if (audioTrack) {
            auto track = MediaStreamTrack::Wrap(audioTrack, _factory);
            CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), track, result, Napi::Value)
            return result;
        }
        auto videoTrack = _stream->FindVideoTrack(label);
        if (videoTrack) {
            auto track = MediaStreamTrack::Wrap(videoTrack, _factory);
            CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), track, result, Napi::Value)
            return result;
        }
        return info.Env().Null();
    }

    Napi::Value MediaStream::AddTrack(const Napi::CallbackInfo& info) {
        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, mediaStreamTrack, napi_ref_ptr<MediaStreamTrack>)
        auto stream = _stream;
        auto track = mediaStreamTrack->track();
        if (track->kind() == track->kAudioKind) {
            stream->AddTrack(webrtc::scoped_refptr<webrtc::AudioTrackInterface> {static_cast<webrtc::AudioTrackInterface*>(track.get())});
        } else {
            stream->AddTrack(webrtc::scoped_refptr<webrtc::VideoTrackInterface> {static_cast<webrtc::VideoTrackInterface*>(track.get())});
        }
        return info.Env().Undefined();
    }

    Napi::Value MediaStream::RemoveTrack(const Napi::CallbackInfo& info) {
        CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, mediaStreamTrack, napi_ref_ptr<MediaStreamTrack>)
        auto stream = _stream;
        auto track = mediaStreamTrack->track();
        if (track->kind() == track->kAudioKind) {
            stream->RemoveTrack(webrtc::scoped_refptr {static_cast<webrtc::AudioTrackInterface*>(track.get())});
        } else {
            stream->RemoveTrack(webrtc::scoped_refptr {static_cast<webrtc::VideoTrackInterface*>(track.get())});
        }
        return info.Env().Undefined();
    }

    Napi::Value MediaStream::Clone(const Napi::CallbackInfo& info) {
        auto clonedStream = _factory->factory()->CreateLocalMediaStream(webrtc::CreateRandomUuid());
        for (auto const& track : this->tracks()) {
            if (track->kind() == track->kAudioKind) {
                auto* audioTrack = static_cast<webrtc::AudioTrackInterface*>(track.get());
                auto* source = audioTrack->GetSource();
                auto clonedTrack = _factory->factory()->CreateAudioTrack(webrtc::CreateRandomUuid(), source);
                clonedStream->AddTrack(clonedTrack);
            } else {
                auto* videoTrack = static_cast<webrtc::VideoTrackInterface*>(track.get());
                auto* source = videoTrack->GetSource();
                auto clonedTrack = _factory->factory()->CreateVideoTrack(webrtc::scoped_refptr {source}, webrtc::CreateRandomUuid());
                clonedStream->AddTrack(clonedTrack);
            }
        }
        auto mediaStream = MediaStream::Wrap(clonedStream, _factory);
        CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), mediaStream, result, Napi::Value)
        return result;
    }

    void MediaStream::Init(Napi::Env env, Napi::Object exports) {
        Napi::HandleScope scope(env);

        Napi::Function func = DefineClass(env,
            "MediaStream",
            {
                InstanceAccessor("id", &MediaStream::GetId, nullptr),
                InstanceAccessor("active", &MediaStream::GetActive, nullptr),
                InstanceMethod("getAudioTracks", &MediaStream::GetAudioTracks),
                InstanceMethod("getVideoTracks", &MediaStream::GetVideoTracks),
                InstanceMethod("getTracks", &MediaStream::GetTracks),
                InstanceMethod("getTrackById", &MediaStream::GetTrackById),
                InstanceMethod("addTrack", &MediaStream::AddTrack),
                InstanceMethod("removeTrack", &MediaStream::RemoveTrack),
                InstanceMethod("clone", &MediaStream::Clone),
            });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("MediaStream", func);
    }
} // namespace node_webrtc
