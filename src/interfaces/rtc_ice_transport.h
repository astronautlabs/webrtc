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

#include <mutex>

#include <node-addon-api/napi.h>
#include <webrtc/api/ice_transport_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/p2p/base/ice_transport_internal.h>

#include "src/enums/node_webrtc/rtc_ice_component.h"
#include "src/node/async_object_wrap_with_loop.h"
#include "src/node/wrap.h"

namespace webrtc {
    class IceTransportInternal;
}

namespace node_webrtc {

    class PeerConnectionFactory;

    class RTCIceTransport : public AsyncObjectWrapWithLoop<RTCIceTransport> {
    public:
        explicit RTCIceTransport(const Napi::CallbackInfo&);

        void Finalize(Napi::Env env) override;

        static void Init(Napi::Env, Napi::Object);

        static Wrap<
            RTCIceTransport*,
            webrtc::scoped_refptr<webrtc::IceTransportInterface>,
            PeerConnectionFactory*>*
        wrap();

        void OnRTCDtlsTransportStopped();

    protected:
        void Stop() override;

    private:
        static Napi::FunctionReference& constructor();

        static RTCIceTransport* Create(
            PeerConnectionFactory*,
            webrtc::scoped_refptr<webrtc::IceTransportInterface>
        );

        void OnStateChanged(webrtc::IceTransportInternal*);
        void OnGatheringStateChanged(webrtc::IceTransportInternal*);

        void TakeSnapshot();

        Napi::Value GetRole(const Napi::CallbackInfo&);
        Napi::Value GetComponent(const Napi::CallbackInfo&);
        Napi::Value GetState(const Napi::CallbackInfo&);
        Napi::Value GetGatheringState(const Napi::CallbackInfo&);

        Napi::Value GetLocalCandidates(const Napi::CallbackInfo&);
        Napi::Value GetRemoteCandidates(const Napi::CallbackInfo&);
        Napi::Value GetSelectedCandidatePair(const Napi::CallbackInfo&);
        Napi::Value GetLocalParameters(const Napi::CallbackInfo&);
        Napi::Value GetRemoteParameters(const Napi::CallbackInfo&);

        RTCIceComponent _component = RTCIceComponent::kRtp;
        PeerConnectionFactory* _factory;
        webrtc::IceGatheringState _gathering_state = webrtc::IceGatheringState::kIceGatheringNew;
        std::mutex _mutex { };
        webrtc::IceRole _role = webrtc::IceRole::ICEROLE_UNKNOWN;
        webrtc::IceTransportState _state = webrtc::IceTransportState::kNew;
        webrtc::scoped_refptr<webrtc::IceTransportInterface> _transport;
    };

} // namespace node_webrtc
