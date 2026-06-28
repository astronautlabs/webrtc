#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/media_stream_interface.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::MediaStreamTrackInterface::TrackState);
} // namespace node_webrtc