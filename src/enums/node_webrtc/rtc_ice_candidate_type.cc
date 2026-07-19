#include "src/enums/node_webrtc/rtc_ice_candidate_type.h"
#include <webrtc/api/candidate.h>
 
namespace node_webrtc {
    static const bidirectional_map<webrtc::IceCandidateType, std::string> IceCandidateTypeMap {
        {
            { webrtc::IceCandidateType::kHost, "host" },
            { webrtc::IceCandidateType::kSrflx, "srflx" },
            { webrtc::IceCandidateType::kRelay, "relay" },
            { webrtc::IceCandidateType::kPrflx, "prflx" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::IceCandidateType, IceCandidateTypeMap)
} // namespace node_webrtc
