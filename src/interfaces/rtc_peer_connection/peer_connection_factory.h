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

#include <memory>
#include <mutex>

#include <node-addon-api/napi.h>
#include <src/rtc_base/physical_socket_server.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>
#include <webrtc/modules/audio_device/include/audio_device.h>

#include "src/functional/maybe.h"

namespace rtc {

    class NetworkManager;
    class PacketSocketFactory;
    class Thread;

} // namespace rtc

namespace webrtc {

    class PeerConnectionFactoryInterface;

} // namespace webrtc

namespace node_webrtc {

    class PeerConnectionFactory
        : public Napi::ObjectWrap<PeerConnectionFactory> {
    public:
        explicit PeerConnectionFactory(const Napi::CallbackInfo&);

        ~PeerConnectionFactory();

        void Destruct();
        void Finalize(Napi::Env env) override;

        /**
         * Get or create the default PeerConnectionFactory. The default uses
         * webrtc::AudioDeviceModule::AudioLayer::kDummyAudio. Call {@link Release} when done.
         */
        static PeerConnectionFactory* GetOrCreateDefault();

        /**
         * Release a reference to the default PeerConnectionFactory.
         */
        static void Release();

        /**
         * Get the underlying webrtc::PeerConnectionFactoryInterface.
         */
        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory() { return _factory; }

        static void Init(Napi::Env, Napi::Object);

        static Napi::FunctionReference& constructor();

        static void Dispose();

        std::unique_ptr<webrtc::Thread> _signalingThread;
        std::unique_ptr<webrtc::Thread> _workerThread;
        std::unique_ptr<webrtc::Thread> _networkThread;

    private:
        bool _destructed = false;
        static PeerConnectionFactory* _default;
        static std::mutex _mutex;
        static int _references;

        webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _factory;
        webrtc::scoped_refptr<webrtc::AudioDeviceModule> _audioDeviceModule;
    };

} // namespace node_webrtc
