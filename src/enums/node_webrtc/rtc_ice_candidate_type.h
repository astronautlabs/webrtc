#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/candidate.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::IceCandidateType)
} // namespace node_webrtc
