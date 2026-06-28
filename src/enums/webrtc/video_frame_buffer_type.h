#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/video/video_frame_buffer.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::VideoFrameBuffer::Type);
} // namespace node_webrtc
