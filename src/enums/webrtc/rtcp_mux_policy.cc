#include "src/enums/webrtc/rtcp_mux_policy.h"
namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::RtcpMuxPolicy, std::string> RtcpMuxPolicyMap {
        {
        { webrtc::PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyNegotiate, "negotiate" },
        { webrtc::PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire, "require" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::RtcpMuxPolicy, RtcpMuxPolicyMap)
} // namespace node_webrtc