#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/rtp_transceiver_interface.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::RtpTransceiverDirection);
} // namespace node_webrtc