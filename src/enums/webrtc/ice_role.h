#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/p2p/base/transport_description.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::IceRole);
} // namespace node_webrtc
