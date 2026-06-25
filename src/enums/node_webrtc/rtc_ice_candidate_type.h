#pragma once

// IWYU pragma: no_include "src/enums/macros/impls.h"

#include "src/enums/enum_class_conversion.h"
#include <src/api/candidate.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::IceCandidateType)
} // namespace node_webrtc

// #define RTC_ICE_CANDIDATE_TYPE IceCandidateType
// #define RTC_ICE_CANDIDATE_TYPE_NAME "IceCandidateType"
// #define RTC_ICE_CANDIDATE_TYPE_LIST \
//   ENUM_SUPPORTED(kHost, "host") \
//   ENUM_SUPPORTED(kSrflx, "srflx") \
//   ENUM_SUPPORTED(kRelay, "relay") \
//   ENUM_SUPPORTED(kPrflx, "prflx")

// #define ENUM(X) RTC_ICE_CANDIDATE_TYPE ## X
// #include "src/enums/macros/def.h"
// #include "src/enums/macros/decls.h"
// #undef ENUM
