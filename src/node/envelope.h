#pragma once
#include <node-addon-api/napi.h>

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
        static T Open(const Napi::Value& napiValue) { 
            auto* envelope = napiValue.As<Napi::External<Napi::Envelope<T>>>().Data();
            T value = envelope->_value;
            delete envelope;
            return value;
        }
        static Napi::External<Envelope<T>> Of(const Napi::Env& env, T value) {
            return Napi::External<Envelope<T>>::New(env, new Envelope<T>(value));
        }
    };

    template <typename T>
    auto CreateEnvelope(Napi::Env env, T value) {
        return Envelope<T>::Of(env, value);
    }
} // namespace Napi