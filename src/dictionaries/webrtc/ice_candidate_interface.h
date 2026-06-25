#pragma once

#include <memory>

#include "src/converters/napi.h"

namespace webrtc { class IceCandidate; }

#define ICE_CANDIDATE_INTERFACE webrtc::IceCandidate*

#define DICT(X) ICE_CANDIDATE_INTERFACE ## X
#include "src/dictionaries/macros/decls.h"
#undef DICT

namespace node_webrtc {

DECLARE_FROM_NAPI(std::shared_ptr<webrtc::IceCandidate>)

}  // namespace node_webrtc
