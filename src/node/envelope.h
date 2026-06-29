#pragma once
#include "src/utilities/nameof.h"
#include <node-addon-api/napi.h>
#include <random>

namespace Napi {
    /**
     * Pass an arbitrary value type as a Napi::External.
     * Usage:
     */
    template <typename T>
    class Envelope {
        explicit Envelope(T value): 
            _value(value) 
        {
        }

        T _value;
    public:
        static bool IsInstance(const Napi::Value& napiValue) { 
            if (!napiValue.IsExternal())
                return false;
            Napi::External external = napiValue.As<Napi::External<Napi::Envelope<T>>>();
            return external.CheckTypeTag(GetTypeTag());
        }

        static T Open(const Napi::Value& napiValue) { 
            if (!IsInstance(napiValue)) {
                Napi::TypeError::New(napiValue.Env(), "Expected Envelope<" + std::string { NAMEOF_TYPE(T) } + ">")
                    .ThrowAsJavaScriptException();
            }

            Napi::External external = napiValue.As<Napi::External<Napi::Envelope<T>>>();
            auto* envelope = external.Data();
            T value = envelope->_value;
            delete envelope;
            return value;
        }

        static Napi::External<Envelope<T>> Of(const Napi::Env& env, T value) {
            Napi::External external = Napi::External<Envelope<T>>::New(env, new Envelope<T>(value));
            external.TypeTag(GetTypeTag());
            return external;
        }

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
    };

    template <typename T>
    auto CreateEnvelope(Napi::Env env, T value) {
        return Envelope<T>::Of(env, value);
    }
} // namespace Napi