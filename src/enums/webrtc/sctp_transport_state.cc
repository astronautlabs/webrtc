#include "src/enums/webrtc/sctp_transport_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::SctpTransportState, std::string> SctpTransportStateMap {
        {
            { webrtc::SctpTransportState::kNew, "connecting" },
            { webrtc::SctpTransportState::kConnecting, "connecting" },
            { webrtc::SctpTransportState::kConnected, "connected" },
            { webrtc::SctpTransportState::kClosed, "closed" },
            { webrtc::SctpTransportState::kNumValues, "num-values" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::SctpTransportState, SctpTransportStateMap)
} // namespace node_webrtc