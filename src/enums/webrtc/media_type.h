#pragma once

#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/media_types.h>

// IWYU pragma: no_include "src/enums/macros/impls.h"


namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::MediaType)
} // namespace node_webrtc

// // FIXME(mroberts): I'm not sure that "data" should be valid.
// #define WEBRTC_MEDIA_TYPE webrtc::MediaType
// #define WEBRTC_MEDIA_TYPE_NAME "kind"
// #define WEBRTC_MEDIA_TYPE_LIST \
//   ENUM_SUPPORTED(WEBRTC_MEDIA_TYPE::MEDIA_TYPE_AUDIO, "audio") \
//   ENUM_SUPPORTED(WEBRTC_MEDIA_TYPE::MEDIA_TYPE_VIDEO, "video") \
//   ENUM_SUPPORTED(WEBRTC_MEDIA_TYPE::MEDIA_TYPE_DATA, "data")

// #define ENUM(X) WEBRTC_MEDIA_TYPE ## X
// #include "src/enums/macros/decls.h"
// #undef ENUM
