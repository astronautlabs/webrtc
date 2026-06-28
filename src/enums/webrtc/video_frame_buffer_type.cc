#include "src/enums/webrtc/video_frame_buffer_type.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::VideoFrameBuffer::Type, std::string> VideoFrameBufferTypeMap {
        {
            { webrtc::VideoFrameBuffer::Type::kNative, "native" },
            { webrtc::VideoFrameBuffer::Type::kI420, "I420" },
            { webrtc::VideoFrameBuffer::Type::kI420A, "I420A" },
            { webrtc::VideoFrameBuffer::Type::kI444, "I444" },
            { webrtc::VideoFrameBuffer::Type::kI010, "I010" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::VideoFrameBuffer::Type, VideoFrameBufferTypeMap)
} // namespace node_webrtc