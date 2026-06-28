#pragma once
#include "src/enums/enum_class_conversion.h"

namespace node_webrtc {
    enum class BinaryType {
        kBlob,
        kArrayBuffer
    };
    DECLARE_ENUM_CLASS_CONVERTER(BinaryType)
} // namespace node_webrtc
