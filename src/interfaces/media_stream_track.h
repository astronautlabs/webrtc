/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <node-addon-api/napi.h>
#include <webrtc/api/media_stream_interface.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/converters.h"
#include "src/converters/napi.h"
#include "src/node/async_object_wrap_with_loop.h"
#include "src/node/wrap.h"
#include "src/utilities/napi_ref_ptr.h"

namespace node_webrtc {

    class PeerConnectionFactory;

    class MediaStreamTrack
        : public AsyncObjectWrapWithLoop<MediaStreamTrack>,
          public webrtc::ObserverInterface {
    public:
        explicit MediaStreamTrack(const Napi::CallbackInfo&);

        void Finalize(Napi::Env env) override;

        static void Init(Napi::Env, Napi::Object);

        // ~ Begin interface webrtc::ObserverInterface
        void OnChanged() override;
        // ~ End interface webrtc::ObserverInterface

        void OnPeerConnectionClosed();

        bool active() { return _ended ? false : _track->state() == webrtc::MediaStreamTrackInterface::TrackState::kLive; }
        PeerConnectionFactory* factory() { return _factory; }
        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track() { return _track; }

        static ::node_webrtc::Wrap<
            MediaStreamTrack*,
            webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>,
            PeerConnectionFactory*>*
        wrap();

        static Napi::FunctionReference& constructor();

        inline std::string getId() {
            return _track->id();
        }

    protected:
        void Stop() override;

    private:
        static MediaStreamTrack* Create(
            PeerConnectionFactory*,
            webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
        );

        Napi::Value GetEnabled(const Napi::CallbackInfo&);
        void SetEnabled(const Napi::CallbackInfo&, const Napi::Value&);
        Napi::Value GetId(const Napi::CallbackInfo&);
        Napi::Value GetKind(const Napi::CallbackInfo&);
        Napi::Value GetReadyState(const Napi::CallbackInfo&);
        Napi::Value GetMuted(const Napi::CallbackInfo&);

        Napi::Value Clone(const Napi::CallbackInfo&);
        Napi::Value JsStop(const Napi::CallbackInfo&);

        bool _ended = false;
        bool _enabled;
        napi_ref_ptr<PeerConnectionFactory> _factory;
        webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> _track;
    };

    DECLARE_CONVERTER(MediaStreamTrack*, webrtc::scoped_refptr<webrtc::AudioTrackInterface>)
    DECLARE_CONVERTER(MediaStreamTrack*, webrtc::scoped_refptr<webrtc::VideoTrackInterface>)

    DECLARE_TO_AND_FROM_NAPI(MediaStreamTrack*)
    DECLARE_FROM_NAPI(webrtc::scoped_refptr<webrtc::AudioTrackInterface>)
    DECLARE_FROM_NAPI(webrtc::scoped_refptr<webrtc::VideoTrackInterface>)

} // namespace node_webrtc
