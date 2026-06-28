#include "src/enums/node_webrtc/rtc_sdp_type.h"

namespace node_webrtc {
    static const bidirectional_map<RTCSdpType, std::string> RTCSdpTypeMap {
        {
            { RTCSdpType::kOffer, "offer" },
            { RTCSdpType::kAnswer, "answer" },
            { RTCSdpType::kPrAnswer, "pranswer" },
            { RTCSdpType::kRollback, "rollback" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(RTCSdpType, RTCSdpTypeMap)
} // namespace node_webrtc