#pragma once

// IWYU pragma: no_include "src/enums/macros/impls.h"

#include "src/enums/enum_class_conversion.h"
#include <src/api/candidate.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::IceCandidateType)
} // namespace node_webrtc
