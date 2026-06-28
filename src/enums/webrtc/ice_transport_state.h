#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/transport/enums.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::IceTransportState);
} // namespace node_webrtc