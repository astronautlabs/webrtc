#pragma once
#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/rtp_parameters.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::DegradationPreference);
} // namespace node_webrtc