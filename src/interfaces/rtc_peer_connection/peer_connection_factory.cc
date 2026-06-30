/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "peer_connection_factory.h"

#include <memory>

#include "src/utilities/log.h"
#include "src/webrtc/test_audio_device_module.h"
#include "src/webrtc/zero_capturer.h"
#include <src/rtc_base/network.h>
#include <webrtc/api/audio/create_audio_device_module.h>
#include <webrtc/api/audio_codecs/builtin_audio_decoder_factory.h>
#include <webrtc/api/audio_codecs/builtin_audio_encoder_factory.h>
#include <webrtc/api/create_peerconnection_factory.h>
#include <webrtc/api/environment/environment_factory.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/video_codecs/builtin_video_decoder_factory.h>
#include <webrtc/api/video_codecs/builtin_video_encoder_factory.h>
#include <webrtc/api/video_codecs/video_decoder_factory.h>
#include <webrtc/api/video_codecs/video_encoder_factory.h>
#include <webrtc/modules/audio_device/include/audio_device.h>
#include <webrtc/modules/audio_device/include/fake_audio_device.h>
#include <webrtc/p2p/base/basic_packet_socket_factory.h>
#include <webrtc/rtc_base/physical_socket_server.h>
#include <webrtc/rtc_base/ssl_adapter.h>
#include <webrtc/rtc_base/thread.h>

namespace node_webrtc {

    Napi::FunctionReference& PeerConnectionFactory::constructor() {
        static Napi::FunctionReference constructor;
        return constructor;
    }

    PeerConnectionFactory* PeerConnectionFactory::_default = nullptr;
    std::mutex PeerConnectionFactory::_mutex {}; // NOLINT
    int PeerConnectionFactory::_references = 0;

    PeerConnectionFactory::PeerConnectionFactory(const Napi::CallbackInfo& info) :
        Napi::ObjectWrap<PeerConnectionFactory>(info) {
        auto env = info.Env();
        bool result = false;

        if (!info.IsConstructCall()) {
            Napi::TypeError::New(
                env, "Use the new operator to construct a PeerConnectionFactory.")
                .ThrowAsJavaScriptException();
            return;
        }

        // TODO(mroberts): Read `audioLayer` from some PeerConnectionFactoryOptions?
        auto audioLayer = MakeNothing<webrtc::AudioDeviceModule::AudioLayer>();

        // Network thread
        _networkThread = webrtc::Thread::CreateWithSocketServer();
        _networkThread->SetName("pcf:network", nullptr);
        _networkThread->Start();

        // Worker thread
        _workerThread = webrtc::Thread::Create();
        _workerThread->SetName("pcf:worker", nullptr);
        _workerThread->Start();
        _workerThread->BlockingCall([&] {
            _audioDeviceModule = audioLayer
                                     .Map([](auto audioLayer) {
                // TODO(mroberts): I'm just trying to get this to compile right
                // now. We need to call something like
                // CreateDefaultzTaskQueueFactory(). This code is currently
                // unused, though.
                return webrtc::CreateAudioDeviceModule(
                    webrtc::CreateEnvironment(), audioLayer);
            })
                                     .Or([] {
                return TestAudioDeviceModule::CreateTestAudioDeviceModule(
                    ZeroCapturer::Create(48000),
                    TestAudioDeviceModule::CreateDiscardRenderer(48000));
            });
        });

        // Signaling thread
        _signalingThread = webrtc::Thread::Create();
        _signalingThread->SetName("pcf:signaling", nullptr);
        _signalingThread->Start();

        // Factory
        _factory = webrtc::CreatePeerConnectionFactory(
            _networkThread.get(),
            _workerThread.get(),
            _signalingThread.get(),
            _audioDeviceModule,
            webrtc::CreateBuiltinAudioEncoderFactory(),
            webrtc::CreateBuiltinAudioDecoderFactory(),
            webrtc::CreateBuiltinVideoEncoderFactory(),
            webrtc::CreateBuiltinVideoDecoderFactory(),
            nullptr,
            nullptr);
        assert(_factory);

        webrtc::PeerConnectionFactoryInterface::Options options;
        options.network_ignore_mask = 0;
        _factory->SetOptions(options);
    }

    PeerConnectionFactory::~PeerConnectionFactory() {
        Log(this, "~PeerConnectionFactory()");
        Destruct();
    }

    void PeerConnectionFactory::Destruct() {
        if (_destructed)
            return;
        Log(this, "Destruct()");
        _factory = nullptr;

        _destructed = true;
        _workerThread->BlockingCall([&] { this->_audioDeviceModule = nullptr; });

        _workerThread->Stop();
        _signalingThread->Stop();
        _networkThread->Stop();

        _workerThread = nullptr;
        _signalingThread = nullptr;
        _networkThread = nullptr;
    }

    void PeerConnectionFactory::Finalize(Napi::Env env) {
        Log(this, "Finalize()");
        Destruct();
    }

    PeerConnectionFactory* PeerConnectionFactory::GetOrCreateDefault() {
        Log<PeerConnectionFactory>("GetOrCreateDefault()");
        _mutex.lock();
        _references++;
        if (_references == 1) {
            assert(_default == nullptr);
            auto env = constructor().Env();
            Napi::HandleScope scope(env);
            auto object = constructor().New({});
            auto* factory = Unwrap(object);
            _default = factory;
            _default->Ref();
        }
        _mutex.unlock();
        return _default;
    }

    void PeerConnectionFactory::Release() {
        Log<PeerConnectionFactory>("Release()");
        _mutex.lock();
        _references--;
        assert(_references >= 0);
        if (!_references) {
            assert(_default != nullptr);
            _default->Destruct();
            _default->Unref();
            _default = nullptr;
        }
        _mutex.unlock();
    }

    void PeerConnectionFactory::Init(Napi::Env env, Napi::Object exports) {
        assert(webrtc::InitializeSSL());

        auto func = DefineClass(env,
            "RTCPeerConnectionFactory",
            {

            });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("RTCPeerConnectionFactory", func);
    }

} // namespace node_webrtc
