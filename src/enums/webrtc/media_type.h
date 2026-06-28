#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/media_types.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::MediaType)
} // namespace node_webrtc
