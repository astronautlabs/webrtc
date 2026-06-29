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
#include <js_native_api_types.h>
#include <node-addon-api/napi.h>
#include <random>
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

        static napi_ref_ptr<ProxyT> UnwrapProxy(const Napi::Value& value) {
            if (!IsInstance(value)) {
                Throw<Napi::TypeError>(value.Env(), "Expected instance of " + ClassName());
                return nullptr;
            }

            return Napi::ObjectWrap<ProxyT>::Unwrap(value.As<Napi::Object>());
        }

        static bool IsInstance(const Napi::Value& value) {
            if (!value.IsObject())
                return false;

            auto object = value.As<Napi::Object>();
            return object.CheckTypeTag(TypeTag());
        }

        webrtc::scoped_refptr<NativeT> Handle() { return _handle; }

    protected:
        virtual void Construct(const Napi::CallbackInfo& info) {
            if (info.Length() != 2 || !info[0].IsObject() || !info[1].IsExternal()) {
                Throw<Napi::TypeError>(info.Env(), "You cannot construct a " + ClassName());
                return;
            }
            _factory = PeerConnectionFactory::Unwrap(info[0].As<Napi::Object>());
            _handle = Napi::Envelope<webrtc::scoped_refptr<NativeT>>::Open(info[1]);
        }

        static napi_ref_ptr<ProxyT> CreateProxy(webrtc::scoped_refptr<NativeT> channel, napi_ref_ptr<PeerConnectionFactory> factory) {
            auto env = constructor().Env();
            Napi::HandleScope scope(env);
            auto object = constructor().New({factory->Value(), Napi::CreateEnvelope(env, channel)});
            auto unwrapped = ProxyT::UnwrapProxy(object);
            unwrapped->Ref();
            return unwrapped;
        }

        static napi_type_tag* TypeTag() {
            static napi_type_tag tag;
            std::random_device rd;
            std::mt19937_64 engine(rd());
            tag.upper = engine();
            tag.lower = engine();
            return &tag;
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

    template <typename ProxyT, typename NativeT>
    concept CCanConvertToNative = requires(ProxyT a) {
        { a.Handle() } -> std::convertible_to<webrtc::scoped_refptr<NativeT>>;
    };

    template <typename NativeT, CCanConvertToNative<NativeT> ProxyT>
    struct Converter<napi_ref_ptr<ProxyT>, webrtc::scoped_refptr<NativeT>> {
        static Validation<webrtc::scoped_refptr<NativeT>> Convert(napi_ref_ptr<ProxyT> proxy) {
            return Validation { proxy->Handle() };
        };
    };

    template <typename NativeT, typename ProxyT>
    concept CCanConvertToProxy = requires(webrtc::scoped_refptr<NativeT> a) {
        { ProxyT::Wrap(a) } -> std::convertible_to<napi_ref_ptr<ProxyT>>;
    };

    template <typename ProxyT, CCanConvertToProxy<ProxyT> NativeT>
    struct Converter<webrtc::scoped_refptr<NativeT>, napi_ref_ptr<ProxyT>> {
        static Validation<napi_ref_ptr<ProxyT>> Convert(webrtc::scoped_refptr<NativeT> native) {
            if (!ProxyT::InstanceOf(native))
                return Validation<napi_ref_ptr<ProxyT>>::Invalid("Not an instance of " + ProxyT::ClassName());
            return Validation { ProxyT::Wrap(native) };
        };
    };

    template <typename T>
    concept CCanConvertToProxyFromNapiValue = requires(const Napi::Value& value) {
        { T::UnwrapProxy(value) } -> std::convertible_to<napi_ref_ptr<T>>;
    };

    template <CCanConvertToProxyFromNapiValue T>
    struct Converter<Napi::Value, napi_ref_ptr<T>> {
        static Validation<napi_ref_ptr<T>> Convert(Napi::Value value) {
            return Validation<napi_ref_ptr<T>> { T::UnwrapProxy(value) };
        };
    };
} // namespace node_webrtc
