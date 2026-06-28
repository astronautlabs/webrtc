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

#include <webrtc/api/data_channel_interface.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/enums/node_webrtc/binary_type.h"
#include "src/node/proxy.h"
#include "src/utilities/napi_ref_ptr.h"

namespace node_webrtc {

    class PeerConnectionFactory;

    /**
     * Represents the RTCDataChannel Javascript object, and manages all state related to webrtc::DataChannelInterface.
     */
    class RTCDataChannel
        : public Proxy<RTCDataChannel, webrtc::DataChannelInterface>,
          public webrtc::DataChannelObserver {
        bool hasOpened = false;

    public:
        explicit RTCDataChannel(const Napi::CallbackInfo&);

        static void Init(Napi::Env, Napi::Object);

        // ~ Begin DataChannelObserver interface.
        void OnStateChange() override;
        void OnMessage(const webrtc::DataBuffer& buffer) override;
        // ~ End DataChannelObserver interface

        void OnPeerConnectionClosed();

    private:
        static void HandleMessage(RTCDataChannel&, const webrtc::DataBuffer& buffer);

        Napi::Value Send(const Napi::CallbackInfo&);
        Napi::Value Close(const Napi::CallbackInfo&);

        Napi::Value GetBufferedAmount(const Napi::CallbackInfo&);
        Napi::Value GetId(const Napi::CallbackInfo&);
        Napi::Value GetLabel(const Napi::CallbackInfo&);
        Napi::Value GetMaxPacketLifeTime(const Napi::CallbackInfo&);
        Napi::Value GetMaxRetransmits(const Napi::CallbackInfo&);
        Napi::Value GetNegotiated(const Napi::CallbackInfo&);
        Napi::Value GetOrdered(const Napi::CallbackInfo&);
        Napi::Value GetPriority(const Napi::CallbackInfo&);
        Napi::Value GetProtocol(const Napi::CallbackInfo&);
        Napi::Value GetBinaryType(const Napi::CallbackInfo&);
        Napi::Value GetReadyState(const Napi::CallbackInfo&);
        void SetBinaryType(const Napi::CallbackInfo&, const Napi::Value&);

        void CleanupInternals();

        BinaryType _binaryType = BinaryType::kArrayBuffer;
        int _cached_id = 0;
        std::string _cached_label;
        uint16_t _cached_max_packet_life_time = 0;
        uint16_t _cached_max_retransmits = 0;
        bool _cached_negotiated = false;
        bool _cached_ordered = false;
        std::string _cached_protocol;
        uint64_t _cached_buffered_amount = 0;
    };

} // namespace node_webrtc
