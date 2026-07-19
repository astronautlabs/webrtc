#include "src/dictionaries/node_webrtc/rtc_session_description_init.h"

#include <webrtc/api/scoped_refptr.h>
#include <utility>
#include <memory>

#include <node-addon-api/napi.h>
#include <webrtc/api/jsep.h>

#include "src/dictionaries/macros/napi.h"
#include "src/functional/curry.h"
#include "src/functional/operators.h"
#include "src/functional/validation.h"

namespace node_webrtc {

#define RTC_SESSION_DESCRIPTION_INIT_FN CreateValidRTCSessionDescriptionInit

static Validation<RTC_SESSION_DESCRIPTION_INIT>
RTC_SESSION_DESCRIPTION_INIT_FN(const RTCSdpType type, const std::string &sdp) {
  return Pure(CreateRTCSessionDescriptionInit(type, sdp));
}

TO_NAPI_IMPL(RTCSessionDescriptionInit, pair) {
  auto env = pair.first;
  Napi::EscapableHandleScope scope(env);
  NODE_WEBRTC_CREATE_OBJECT_OR_RETURN(env, object)
  NODE_WEBRTC_CONVERT_AND_SET_OR_RETURN(env, object, "type", pair.second.type)
  NODE_WEBRTC_CONVERT_AND_SET_OR_RETURN(env, object, "sdp", pair.second.sdp)
  return Pure(scope.Escape(object));
}

CONVERTER_IMPL(RTCSessionDescriptionInit,
               std::shared_ptr<webrtc::SessionDescriptionInterface>,
               init) {
  webrtc::SdpType type_ {};
  switch (init.type) {
  case RTCSdpType::kOffer:
    type_ = webrtc::SdpType::kOffer;
    break;
  case RTCSdpType::kPrAnswer:
    type_ = webrtc::SdpType::kPrAnswer;
    break;
  case RTCSdpType::kAnswer:
    type_ = webrtc::SdpType::kAnswer;
    break;
  case RTCSdpType::kRollback:
    type_ = webrtc::SdpType::kRollback;
  }
  webrtc::SdpParseError error;
  auto description = webrtc::CreateSessionDescription(type_, init.sdp, &error);
  if (!description) {
    return Validation<std::shared_ptr<
        webrtc::SessionDescriptionInterface>>::Invalid(error.description);
  }

  return Pure(std::shared_ptr<webrtc::SessionDescriptionInterface> { std::move(description) });
}

CONVERTER_IMPL(std::shared_ptr<const webrtc::SessionDescriptionInterface>,
               RTCSessionDescriptionInit, description) {
  if (!description) {
    return Validation<RTCSessionDescriptionInit>::Invalid(
        "RTCSessionDescription is null");
  }
  std::string sdp;
  if (!description->ToString(&sdp)) {
    return Validation<RTCSessionDescriptionInit>::Invalid(
        "Failed to print the SDP. This is pretty weird. File a bug on "
        "https://github.com/astronautlabs/webrtc");
  }
  return curry(CreateRTCSessionDescriptionInit) %
         From<RTCSdpType>(description->type()) * Pure(sdp);
}

FROM_NAPI_IMPL(std::shared_ptr<webrtc::SessionDescriptionInterface>,
               pair) {
  return From<RTCSessionDescriptionInit>(pair)
      .FlatMap<std::shared_ptr<webrtc::SessionDescriptionInterface>>(
          Converter<RTCSessionDescriptionInit,
                    std::shared_ptr<
                        webrtc::SessionDescriptionInterface>>::Convert);
}

TO_NAPI_IMPL(std::shared_ptr<const webrtc::SessionDescriptionInterface>, pair) {
  return From<RTCSessionDescriptionInit>(pair.second)
      .FlatMap<Napi::Value>([env = pair.first](auto value) {
        return From<Napi::Value>(std::make_pair(env, value));
      });
}

} // namespace node_webrtc

#define DICT(X) RTC_SESSION_DESCRIPTION_INIT##X
#include "src/dictionaries/macros/impls.h"
#undef DICT
