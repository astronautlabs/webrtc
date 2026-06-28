#pragma once
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class RTCDtxStatus: std::uint8_t {
        kDisabled,
        kEnabled
    };

    DECLARE_ENUM_CLASS_CONVERTER(RTCDtxStatus)
} // namespace node_webrtc