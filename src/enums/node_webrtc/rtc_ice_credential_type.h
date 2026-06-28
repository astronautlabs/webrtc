#pragma once
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class RTCIceCredentialType: std::uint8_t {
        kPassword,
        kOAuth
    };

    DECLARE_ENUM_CLASS_CONVERTER(RTCIceCredentialType)
} // namespace node_webrtc
