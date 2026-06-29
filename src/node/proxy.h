/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include "src/functional/validation.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/node/utility.h"
#include "src/utilities/nameof.h"
#include "src/node/async_object_wrap_with_loop.h"
#include "src/node/envelope.h"
#include "src/node/proxy_registry.h"
#include "src/utilities/bidi_map.h"
#include "src/utilities/napi_ref_ptr.h"
#include <concepts>
#include <node-addon-api/napi.h>
#include <src/api/scoped_refptr.h>
#include <string_view>

namespace node_webrtc {
    /**
     * Maintains a map between native webrtc objects and their node-webrtc proxies. 
     */
    template <typename ProxyT, typename NativeT>
    class Proxy: public AsyncObjectWrapWithLoop<ProxyT> {
    public:
        Proxy(const Napi::CallbackInfo& info):
            AsyncObjectWrapWithLoop<ProxyT>(ClassName(), static_cast<ProxyT&>(*this), info)
        {
        }

    protected:
        virtual void Construct(const Napi::CallbackInfo& info) {
            _factory = PeerConnectionFactory::Unwrap(info[0].As<Napi::Object>());
            _handle = Napi::Envelope<webrtc::scoped_refptr<NativeT>>::Open(info[1]);
        }

        static napi_ref_ptr<ProxyT> CreateProxy(webrtc::scoped_refptr<NativeT> channel, napi_ref_ptr<PeerConnectionFactory> factory) {
            auto env = constructor().Env();
            Napi::HandleScope scope(env);
            auto object = constructor().New({factory->Value(), Napi::CreateEnvelope(env, channel)});
            auto* unwrapped = ProxyT::Unwrap(object);
            unwrapped->Ref();
            return unwrapped;
        }

        static std::string ClassName() {
            return std::string { NAMEOF_TYPE(ProxyT) };
        }

        void UnregisterProxy() {
            registry().Release(static_cast<ProxyT*>(this));
        }

        webrtc::scoped_refptr<NativeT> _handle;
        napi_ref_ptr<PeerConnectionFactory> _factory;

        static Napi::FunctionReference& constructor() {
            static Napi::FunctionReference constructor;
            return constructor;
        }

        static auto& registry() {
            static ::node_webrtc::ProxyRegistry<NativeT, ProxyT> registry { CreateProxy };
            return registry;
        }
    public:
        static napi_ref_ptr<ProxyT> Wrap(
            webrtc::scoped_refptr<NativeT> object, 
            napi_ref_ptr<PeerConnectionFactory> factory = PeerConnectionFactory::GetOrCreateDefault()
        ) {
            return ProxyT::registry().Proxy(object, factory);
        }

        void Finalize(Napi::Env env) override {
            UnregisterProxy();
            _handle = nullptr;
            _factory = nullptr;
        }
    };

} // namespace node_webrtc
