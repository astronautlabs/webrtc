/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include "src/converters.h"
#include "src/functional/validation.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory_reference.h"
#include "src/node/utility.h"
#include "src/utilities/bundle.h"
#include "src/utilities/nameof.h"
#include "src/utilities/log.h"
#include "src/node/async_object_wrap_with_loop.h"
#include "src/node/envelope.h"
#include "src/utilities/bidi_map.h"
#include "src/utilities/napi_ref_ptr.h"
#include "src/webrtc_addon.h"
#include <concepts>
#include <js_native_api_types.h>
#include <node-addon-api/napi.h>
#include <random>
#include <src/api/scoped_refptr.h>

namespace node_webrtc {
    /**
     * Maintains a map between native webrtc objects and their node-webrtc proxies. 
     */
    template <typename ProxyT, typename NativeT>
    class Proxy: public AsyncObjectWrapWithLoop<ProxyT> {
    public:
        Proxy(const Napi::CallbackInfo& info):
            AsyncObjectWrapWithLoop<ProxyT>(className(), static_cast<ProxyT&>(*this), info)
        {
            this->Value().TypeTag(GetTypeTag());
        }

        static std::vector<napi_ref_ptr<ProxyT>> UnwrapArray(const Napi::Value& value) {
            return ToOrThrow<std::vector<napi_ref_ptr<ProxyT>>>(value.Env(), value, "Expected an array of " + className());
        }

        static napi_ref_ptr<ProxyT> UnwrapProxy(const Napi::Value& value) {
            if (!IsInstance(value)) {
                Throw<Napi::TypeError>(value.Env(), "Expected instance of " + className());
                return nullptr;
            }

            return Napi::ObjectWrap<ProxyT>::Unwrap(value.As<Napi::Object>());
        }

        static bool IsInstance(const Napi::Value& value) {
            if (!value.IsObject())
                return false;

            auto object = value.As<Napi::Object>();
            return object.CheckTypeTag(GetTypeTag());
        }

        static std::string className() {
            return std::string { NAMEOF_TYPE(ProxyT) };
        }

        webrtc::scoped_refptr<NativeT> handle() { return _handle; }

    protected:
        virtual void Construct(const Napi::CallbackInfo& info) {
            using HandleLetter = Napi::Envelope<webrtc::scoped_refptr<NativeT>>;
            using ArgsLetter = Napi::Envelope<Bundle>;

            if (info.Length() != 2 || !HandleLetter::IsInstance(info[0]) || !ArgsLetter::IsInstance(info[1])) {
                Log(this, "Invalid construction for " + className() + ", throwing exception");
                Throw<Napi::TypeError>(info.Env(), "Invalid construction for " + className());
                return;
            }

            ConstructFromHandle(
                HandleLetter::Open(info[0]), 
                ArgsLetter::Open(info[1])
            );
        }

        virtual void ConstructFromHandle(webrtc::scoped_refptr<NativeT> handle, Bundle args) {
            _handle = handle;
            assert(_handle);

            _factory = args.Fragment<PeerConnectionFactoryReference>().factory();
            assert(_factory);
            
        }

        /**
        * Maintains a map between native webrtc objects and their node-webrtc proxies. 
        */
        class Registry {
        public:
            explicit Registry(Napi::Env env):
                _env(env)
            {
            }

            Registry(Registry const&) = delete;
            Registry& operator=(Registry const&) = delete;

            Napi::Env _env;

            napi_ref_ptr<ProxyT> Proxy(webrtc::scoped_refptr<NativeT> key, Bundle args) {
                auto factory = args.Fragment<PeerConnectionFactoryReference>().factory();
                if (!factory)
                    factory = PeerConnectionFactory::GetOrCreateDefault();

                return _map.computeIfAbsent(key, [this, key, factory]() {
                    Napi::HandleScope scope(_env);
                    return ProxyT::UnwrapProxy(
                        constructor(_env).New({
                            Napi::CreateEnvelope(_env, key),
                            Napi::CreateEnvelope(_env, Bundle {}.AddFragment(PeerConnectionFactoryReference { factory ? factory : PeerConnectionFactory::GetOrCreateDefault() }))
                        })
                    );
                });
            }

            napi_ref_ptr<ProxyT> GetProxy(webrtc::scoped_refptr<NativeT> key) {
                return _map.has(key) ? _map.get(key).UnsafeFromJust() : nullptr;
            }

            webrtc::scoped_refptr<NativeT> Native(napi_ref_ptr<ProxyT> key) {
                return _map.get(key);
            }

            void Release(napi_ref_ptr<ProxyT> value) {
                _map.reverseRemove(value);
            }

        private:
            BidiMap<webrtc::scoped_refptr<NativeT>, napi_ref_ptr<ProxyT>> _map;
        };

        static napi_type_tag* GetTypeTag() {
            static napi_type_tag tag;
            if (tag.lower == 0 && tag.upper == 0) {
                std::random_device rd;
                std::mt19937_64 engine(rd());
                tag.upper = engine();
                tag.lower = engine();
            }
            return &tag;
        }

        void UnregisterProxy() {
            registry(this->Env()).Release(static_cast<ProxyT*>(this));
        }

        webrtc::scoped_refptr<NativeT> _handle;
        napi_ref_ptr<PeerConnectionFactory> _factory;

        class JSConstructor: public Napi::FunctionReference {};

        static Napi::FunctionReference& constructor(Napi::Env env) {
            return Addon::Fragment<JSConstructor>(env);
        }

        static Registry& registry(Napi::Env env) {
            return Addon::Fragment<Registry>(env);
        }
    public:
        static napi_ref_ptr<ProxyT> Wrap(
            Napi::Env env,
            webrtc::scoped_refptr<NativeT> object, 
            Bundle args
        ) {
            return ProxyT::registry(env).Proxy(object, args);
        }

        static napi_ref_ptr<ProxyT> GetProxy(
            Napi::Env env,
            webrtc::scoped_refptr<NativeT> object
        ) {
            return ProxyT::registry(env).GetProxy(object);
        }

        void Finalize(Napi::Env env) override {
            this->Stop();
            UnregisterProxy();
            _handle = nullptr;
            _factory = nullptr;
        }
    };

    // Convert ProxyT -> NativeT

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

    // Convert NativeT -> ProxyT

    template <typename NativeT, typename ProxyT>
    concept CCanConvertToProxy = requires(webrtc::scoped_refptr<NativeT> a) {
        { ProxyT::Wrap(a) } -> std::convertible_to<napi_ref_ptr<ProxyT>>;
    };

    template <typename ProxyT, CCanConvertToProxy<ProxyT> NativeT>
    struct Converter<webrtc::scoped_refptr<NativeT>, napi_ref_ptr<ProxyT>> {
        static Validation<napi_ref_ptr<ProxyT>> Convert(webrtc::scoped_refptr<NativeT> native) {
            if (!ProxyT::InstanceOf(native))
                return Validation<napi_ref_ptr<ProxyT>>::Invalid("Not an instance of " + ProxyT::className());
            return Validation { ProxyT::Wrap(native) };
        };
    };

    // Convert Napi::Value -> ProxyT

    template <typename T>
    concept CCanConvertToProxyFromNapiValue = requires(const Napi::Value& value) {
        { T::UnwrapProxy(value) } -> std::convertible_to<napi_ref_ptr<T>>;
    };

    template <CCanConvertToProxyFromNapiValue T>
    struct Converter<Napi::Value, napi_ref_ptr<T>> {
        static Validation<napi_ref_ptr<T>> Convert(Napi::Value value) {
            if (!T::IsInstance(value))
                return Validation<napi_ref_ptr<T>>::Invalid("Expected instance of " + T::className() + " while converting");
            return Validation<napi_ref_ptr<T>> { T::UnwrapProxy(value) };
        };
    };

} // namespace node_webrtc
