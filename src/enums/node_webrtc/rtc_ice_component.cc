#include "src/enums/node_webrtc/rtc_ice_component.h"

namespace node_webrtc {
    static const bidirectional_map<RTCIceComponent, std::string> RTCIceComponentMap {
        {
            { RTCIceComponent::kRtp, "rtp" },
            { RTCIceComponent::kRtcp, "rtcp" }
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(RTCIceComponent, RTCIceComponentMap)
} // namespace node_webrtc