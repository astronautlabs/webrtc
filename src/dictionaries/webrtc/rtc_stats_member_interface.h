#pragma once

#include <webrtc/api/stats/attribute.h>
#include "src/converters/napi.h"

namespace webrtc { class RTCStatsMemberInterface; }
//namespace webrtc { class Attribute; }

namespace node_webrtc {
    DECLARE_TO_NAPI(webrtc::Attribute)

}  // namespace node_webrtc
