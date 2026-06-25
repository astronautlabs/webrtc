#include "src/enums/webrtc/media_type.h"
#include <src/api/media_types.h>

namespace node_webrtc {
    static const bidirectional_map<webrtc::MediaType, std::string> MediaTypeMap {
        {
            { webrtc::MediaType::AUDIO,         "audio" },
            { webrtc::MediaType::VIDEO,         "video" },
            { webrtc::MediaType::DATA,          "data" },
            { webrtc::MediaType::UNSUPPORTED,   "unsupported" },
            { webrtc::MediaType::ANY,           "any" }
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::MediaType, MediaTypeMap)
} // namespace node_webrtc

// #define ENUM(X) WEBRTC_MEDIA_TYPE ## X
// #include "src/enums/macros/impls.h"
// #undef ENUM
