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
#include <webrtc/api/scoped_refptr.h>
#include "src/node/proxy.h"

namespace webrtc {
    class MediaStreamInterface;
    class MediaStreamTrackInterface;
}

namespace node_webrtc {
    class MediaStreamTrack;
    class PeerConnectionFactory;
    struct RTCMediaStreamInit;

    class MediaStream: public Proxy<MediaStream, webrtc::MediaStreamInterface> {
    public:
        MediaStream(const Napi::CallbackInfo&);
        void Construct(const Napi::CallbackInfo &info) override;
        static void Init(Napi::Env, Napi::Object);
        void Finalize(Napi::Env env) override;

        std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> tracks();

    private:
        Napi::Value GetId(const Napi::CallbackInfo&);
        Napi::Value GetActive(const Napi::CallbackInfo&);
        Napi::Value GetAudioTracks(const Napi::CallbackInfo&);
        Napi::Value GetVideoTracks(const Napi::CallbackInfo&);
        Napi::Value GetTracks(const Napi::CallbackInfo&);
        Napi::Value GetTrackById(const Napi::CallbackInfo&);
        Napi::Value AddTrack(const Napi::CallbackInfo&);
        Napi::Value RemoveTrack(const Napi::CallbackInfo&);
        Napi::Value Clone(const Napi::CallbackInfo&);
        
        bool _shouldReleaseFactory;
    };
} // namespace node_webrtc
