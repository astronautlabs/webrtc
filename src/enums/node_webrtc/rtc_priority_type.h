#pragma once
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class RTCPriorityType: std::uint8_t {
        kVeryLow,
        kLow,
        kMedium,
        kHigh,
    };

    DECLARE_ENUM_CLASS_CONVERTER(RTCPriorityType)
} // namespace node_webrtc
