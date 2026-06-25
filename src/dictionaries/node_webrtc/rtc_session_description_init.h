#pragma once

#include <iosfwd>
#include <string>

#include <webrtc/api/scoped_refptr.h>
#include "src/converters.h"
#include "src/enums/node_webrtc/rtc_sdp_type.h"

namespace webrtc { class SessionDescriptionInterface; }

// IWYU pragma: no_forward_declare node_webrtc::RTCSessionDescriptionInit
// IWYU pragma: no_include "src/dictionaries/macros/impls.h"

#define RTC_SESSION_DESCRIPTION_INIT RTCSessionDescriptionInit
#define RTC_SESSION_DESCRIPTION_INIT_LIST \
  DICT_REQUIRED(RTCSdpType, type, "type") \
  DICT_DEFAULT(std::string, sdp, "sdp", "")

#define DICT(X) RTC_SESSION_DESCRIPTION_INIT ## X
#include "src/dictionaries/macros/def.h"
#include "src/dictionaries/macros/decls.h"
#undef DICT

namespace node_webrtc {

static inline RTC_SESSION_DESCRIPTION_INIT CreateRTCSessionDescriptionInit(
    const RTCSdpType type,
    const std::string& sdp) {
  return {.type=type, .sdp=sdp};
}

DECLARE_CONVERTER(RTCSessionDescriptionInit, std::shared_ptr<webrtc::SessionDescriptionInterface>)
DECLARE_CONVERTER(std::shared_ptr<const webrtc::SessionDescriptionInterface>, RTCSessionDescriptionInit)

DECLARE_FROM_NAPI(std::shared_ptr<webrtc::SessionDescriptionInterface>)
DECLARE_TO_NAPI(std::shared_ptr<const webrtc::SessionDescriptionInterface>)

}  // namespace node_webrtc
