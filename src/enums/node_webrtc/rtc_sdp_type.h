#pragma once
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class RTCSdpType: std::uint8_t {
        kOffer,
        kAnswer,
        kPrAnswer,
        kRollback,
    };

    DECLARE_ENUM_CLASS_CONVERTER(RTCSdpType)
} // namespace node_webrtc
