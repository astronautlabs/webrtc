#pragma once

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <node-addon-api/napi.h>

#include "src/converters.h"
#include "src/converters/macros.h"
#include "src/functional/maybe.h"
#include "src/functional/validation.h"
#include "src/utilities/napi_ref_ptr.h"

#define CONVERT_OR_THROW_AND_RETURN_NAPI(E, I, O, T)                                                             \
    auto NODE_WEBRTC_UNIQUE_NAME(validation) = From<detail::argument_type<void(T)>::type>(std::make_pair(E, I)); \
    if (NODE_WEBRTC_UNIQUE_NAME(validation).IsInvalid()) {                                                       \
        auto error = NODE_WEBRTC_UNIQUE_NAME(validation).ToErrors()[0];                                          \
        Napi::TypeError::New(E, error).ThrowAsJavaScriptException();                                             \
        return E.Undefined();                                                                                    \
    }                                                                                                            \
    auto O = NODE_WEBRTC_UNIQUE_NAME(validation).UnsafeFromValid();

#define CONVERT_OR_THROW_AND_RETURN_VOID_NAPI(E, I, O, T)                                                        \
    auto NODE_WEBRTC_UNIQUE_NAME(validation) = From<detail::argument_type<void(T)>::type>(std::make_pair(E, I)); \
    if (NODE_WEBRTC_UNIQUE_NAME(validation).IsInvalid()) {                                                       \
        auto error = NODE_WEBRTC_UNIQUE_NAME(validation).ToErrors()[0];                                          \
        Napi::TypeError::New(E, error).ThrowAsJavaScriptException();                                             \
        return;                                                                                                  \
    }                                                                                                            \
    auto O = NODE_WEBRTC_UNIQUE_NAME(validation).UnsafeFromValid();

#define CONVERT_OR_REJECT_AND_RETURN_NAPI(D, I, O, T)                                                                  \
    auto NODE_WEBRTC_UNIQUE_NAME(validation) = From<detail::argument_type<void(T)>::type>(std::make_pair(D.Env(), I)); \
    if (NODE_WEBRTC_UNIQUE_NAME(validation).IsInvalid()) {                                                             \
        auto error = NODE_WEBRTC_UNIQUE_NAME(validation).ToErrors()[0];                                                \
        D.Reject(Napi::TypeError::New(D.Env(), error).Value());                                                        \
        return D.Env().Undefined();                                                                                    \
    }                                                                                                                  \
    auto O = NODE_WEBRTC_UNIQUE_NAME(validation).UnsafeFromValid();

namespace node_webrtc {

/**
 * Declares a converter function that can produce a Javascript value (Napi::Value) for a given C++ type.
 * This goes in the header. It is implemented in the corresponding compilation unit via a TO_NAPI_IMPL macro.
 * @param T The type being converted from.
 */
#define DECLARE_TO_NAPI(T) DECLARE_CONVERTER(std::pair<Napi::Env COMMA T>, Napi::Value)

/**
 * Declares a converter function that can produce a C++ object for a given Javascript value (Napi::Value).
 * This goes in the header. It is implemented in the corresponding compilation unit via a FROM_NAPI_IMPL macro.
 * @param T The type being converted to
 */
#define DECLARE_FROM_NAPI(T) DECLARE_CONVERTER(Napi::Value, T)

#define DECLARE_TO_AND_FROM_NAPI(T) \
    DECLARE_TO_NAPI(T)              \
    DECLARE_FROM_NAPI(T)

/**
 * Provide the implementation for a converter function declared with DECLARE_TO_NAPI which can create a Javascript value (Napi::Value) from objects
 * of the given type.
 *
 * Example:
 * TO_NAPI_IMPL(const webrtc::RTCStatsMemberInterface*, pair) {
 *     // Implement conversion
 *     return ...; // an object of Napi::Value
 * }
 *
 * @param T The type to convert
 * @param V The name of the parameter for the conversion function.
 */
#define TO_NAPI_IMPL(T, V) CONVERTER_IMPL(std::pair<Napi::Env COMMA T>, Napi::Value, V)

/**
 * Define a conversion function for converting a Javascript value (Napi::Value) into a corresponding C++ object.
 * This is the implementation part of DECLARE_FROM_NAPI().
 * @param T
 * @param V
 */
#define FROM_NAPI_IMPL(T, V) CONVERTER_IMPL(Napi::Value, T, V)

    // Conversions for primitives, fundamental types, and identity conversions

    DECLARE_TO_AND_FROM_NAPI(bool)
    DECLARE_TO_AND_FROM_NAPI(double)
    DECLARE_TO_AND_FROM_NAPI(uint8_t)
    DECLARE_TO_AND_FROM_NAPI(uint16_t)
    DECLARE_TO_AND_FROM_NAPI(uint32_t)
    DECLARE_TO_AND_FROM_NAPI(uint64_t)
    DECLARE_TO_AND_FROM_NAPI(int8_t)
    DECLARE_TO_AND_FROM_NAPI(int16_t)
    DECLARE_TO_AND_FROM_NAPI(int32_t)
    DECLARE_TO_AND_FROM_NAPI(int64_t)
    DECLARE_TO_AND_FROM_NAPI(std::string)
    DECLARE_TO_NAPI(Napi::Error)
    DECLARE_FROM_NAPI(Napi::Function)
    DECLARE_FROM_NAPI(Napi::Object)
    DECLARE_TO_NAPI(Napi::Value)
    DECLARE_TO_NAPI(std::vector<bool>)
    DECLARE_FROM_NAPI(Napi::ArrayBuffer)

    template <typename T>
    struct Converter<Napi::Value, Maybe<T>> {
        static Validation<Maybe<T>> Convert(const Napi::Value value) {
            return value.IsUndefined()
                ? Pure(MakeNothing<T>())
                : From<T>(value).Map(&MakeJust<T>);
        }
    };

    template <typename T>
    struct Converter<std::pair<Napi::Env, Maybe<T>>, Napi::Value> {
        static Validation<Napi::Value> Convert(const std::pair<Napi::Env, Maybe<T>> pair) {
            return pair.second.IsJust()
                ? From<Napi::Value>(std::make_pair(pair.first, pair.second.UnsafeFromJust()))
                : Pure(pair.first.Null());
        }
    };

    template <>
    struct Converter<Napi::Value, Napi::Array> {
        static Validation<Napi::Array> Convert(Napi::Value value) {
            return value.IsArray()
                ? Pure(value.As<Napi::Array>())
                : Validation<Napi::Array>::Invalid(("Expected an array"));
        }
    };

    template <>
    struct Converter<std::pair<Napi::Env, Napi::String>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, Napi::String> pair) {
            return Validation<Napi::Value> { pair.second };
        }
    };

    template <>
    struct Converter<std::pair<Napi::Env, Napi::Object>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, Napi::Object> pair) {
            return Validation<Napi::Value> { pair.second };
        }
    };

    template <typename T>
    concept CCanConvertToNapiValue = requires(T a) {
        { a.Value() } -> std::convertible_to<Napi::Value>;
    };

    template <CCanConvertToNapiValue T>
    struct Converter<std::pair<Napi::Env, napi_ref_ptr<T>>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, napi_ref_ptr<T>> pair) {
            return Validation<Napi::Value> { pair.second->Value() };
        };
    };

    template <CCanConvertToNapiValue T>
    struct Converter<std::pair<Napi::Env, T*>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, T*> pair) {
            return Validation<Napi::Value> { pair.second->Value() };
        };
    };

    /**
     * Converts Napi::Array into std::vector, calling back into the conversion system for converting each element of the
     * vector.
     *
     * @tparam T
     */
    template <typename T>
    struct Converter<Napi::Array, std::vector<T>> {
        static Validation<std::vector<T>> Convert(const Napi::Array array) {
            std::vector<T> validated;
            validated.reserve(array.Length());
            for (uint32_t i = 0; i < array.Length(); i++) {
                auto maybeValue = array.Get(i);
                if (maybeValue.Env().IsExceptionPending()) {
                    return Validation<std::vector<T>>::Invalid(maybeValue.Env().GetAndClearPendingException().Message());
                }
                auto maybeValidated = From<T>(maybeValue);
                if (maybeValidated.IsInvalid()) {
                    return Validation<std::vector<T>>::Invalid(maybeValidated.ToErrors());
                }
                validated.push_back(maybeValidated.UnsafeFromValid());
            }
            return Pure(validated);
        }
    };

    template <typename T>
    struct Converter<Napi::Value, std::vector<T>> {
        static Validation<std::vector<T>> Convert(const Napi::Value value) {
            return Converter<Napi::Value, Napi::Array>::Convert(value).FlatMap<std::vector<T>>(Converter<Napi::Array, std::vector<T>>::Convert);
        }
    };

    /**
     * Convert a std::vector to a Napi::Value within the provided Node environment.
     * @tparam T
     */
    template <typename T>
    struct Converter<std::pair<Napi::Env, std::vector<T>>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, std::vector<T>> pair) {
            auto env = pair.first;
            Napi::EscapableHandleScope scope(env);
            auto values = pair.second;
            auto maybeArray = Napi::Array::New(env, values.size());
            if (maybeArray.Env().IsExceptionPending()) {
                return Validation<Napi::Value>::Invalid(maybeArray.Env().GetAndClearPendingException().Message());
            }
            uint32_t i = 0;
            for (const auto& value : values) {
                auto maybeValue = From<Napi::Value>(std::make_pair(env, value));
                if (maybeValue.IsInvalid()) {
                    return Validation<Napi::Value>::Invalid(maybeValue.ToErrors());
                }
                maybeArray.Set(i++, maybeValue.UnsafeFromValid());
                if (maybeArray.Env().IsExceptionPending()) {
                    return Validation<Napi::Value>::Invalid(maybeArray.Env().GetAndClearPendingException().Message());
                }
            }
            return Pure(scope.Escape(maybeArray));
        }
    };

    /**
     * Convert a std::map with string keys to a Napi::Value within the provided Node environment. The result will be an
     * object.
     * @tparam T
     */
    template <typename T>
    struct Converter<std::pair<Napi::Env, std::map<std::string, T>>, Napi::Value> {
        static Validation<Napi::Value> Convert(std::pair<Napi::Env, std::map<std::string, T>> pair) {
            auto env = pair.first;
            Napi::EscapableHandleScope scope(env);
            auto values = pair.second;
            auto maybeObject = Napi::Object::New(env);

            if (env.IsExceptionPending()) {
                return Validation<Napi::Value>::Invalid(env.GetAndClearPendingException().Message());
            }

            // Convert the elements of the map into object properties
            for (const auto& entry : values) {
                auto maybeValue = From<Napi::Value>(std::make_pair(env, entry.second));
                if (maybeValue.IsInvalid()) {
                    return Validation<Napi::Value>::Invalid(maybeValue.ToErrors());
                }
                maybeObject.Set(entry.first, maybeValue.UnsafeFromValid());
                if (env.IsExceptionPending()) {
                    return Validation<Napi::Value>::Invalid(env.GetAndClearPendingException().Message());
                }
            }
            return Pure(scope.Escape(maybeObject));
        }
    };

    template <typename T>
    struct Converter<Napi::Value, Napi::External<T>> {
        static Validation<Napi::External<T>> Convert(Napi::Value value) {
            return value.IsExternal()
                ? Pure(value.As<Napi::External<T>>())
                : Validation<Napi::External<T>>::Invalid("Expected an external");
        }
    };

} // namespace node_webrtc
