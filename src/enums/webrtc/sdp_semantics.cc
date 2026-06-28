#include "src/enums/webrtc/sdp_semantics.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::SdpSemantics, std::string> SdpSemanticsMap {
        {
        { webrtc::SdpSemantics::kPlanB, "plan-b" },
        { webrtc::SdpSemantics::kUnifiedPlan, "unified-plan" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::SdpSemantics, SdpSemanticsMap)
} // namespace node_webrtc

