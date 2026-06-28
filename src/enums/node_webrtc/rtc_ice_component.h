#pragma once
#include <cstdint>
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class RTCIceComponent: uint8_t {
        kRtp,
        kRtcp
    };
    DECLARE_ENUM_CLASS_CONVERTER(RTCIceComponent)
} // namespace node_webrtc