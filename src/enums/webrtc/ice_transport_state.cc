#include "src/enums/webrtc/ice_transport_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::IceTransportState, std::string> IceTransportStateMap {
        {
            { webrtc::IceTransportState::kNew, "new" },
            { webrtc::IceTransportState::kChecking, "checking" },
            { webrtc::IceTransportState::kConnected, "connected" },
            { webrtc::IceTransportState::kCompleted, "completed" },
            { webrtc::IceTransportState::kFailed, "failed" },
            { webrtc::IceTransportState::kDisconnected, "disconnected" },
            { webrtc::IceTransportState::kClosed, "closed" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::IceTransportState, IceTransportStateMap)
} // namespace node_webrtc
