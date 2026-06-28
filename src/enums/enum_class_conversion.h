#include "src/converters.h"
#include "src/converters/napi.h"
#include "src/functional/validation.h"
#include "src/utilities/static_bidirectional_map.h"

#define DECLARE_ENUM_CLASS_CONVERTER(EnumClass) \
    DECLARE_CONVERTER(EnumClass, std::string)   \
    DECLARE_CONVERTER(std::string, EnumClass)   \
    DECLARE_TO_AND_FROM_NAPI(EnumClass)         \
    std::string to_string(const EnumClass& value);

#define ENUM_CLASS_CONVERTER_IMPL(EnumClass, Mappings)                                              \
    CONVERTER_IMPL(EnumClass, std::string, value) {                                                 \
        static bidirectional_map<EnumClass, std::string> map {Mappings};                            \
        return Validation {map[value]};                                                             \
    };                                                                                              \
    CONVERTER_IMPL(std::string, EnumClass, value) {                                                 \
        static bidirectional_map<EnumClass, std::string> map {Mappings};                            \
        return Validation {map[value]};                                                             \
    };                                                                                              \
    CONVERT_VIA(Napi::Value, std::string, EnumClass)                                                \
    TO_NAPI_IMPL(EnumClass, pair) {                                                                 \
        return From<std::string>(pair.second).FlatMap<Napi::Value>([env = pair.first](auto value) { \
            return From<Napi::Value>(std::make_pair(env, value));                                   \
        });                                                                                         \
    };                                                                                              \
    std::string to_string(const EnumClass& value) {                                                 \
        return From<std::string>(value).UnsafeFromValid();                                          \
    }\
