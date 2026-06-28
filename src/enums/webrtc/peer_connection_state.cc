#include "src/enums/webrtc/peer_connection_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::PeerConnectionState, std::string> PeerConnectionStateMap {
        {
            { webrtc::PeerConnectionInterface::PeerConnectionState::kNew, "new" },
            { webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting, "connecting" },
            { webrtc::PeerConnectionInterface::PeerConnectionState::kConnected, "connected" },
            { webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected, "disconnected" },
            { webrtc::PeerConnectionInterface::PeerConnectionState::kFailed, "failed" },
            { webrtc::PeerConnectionInterface::PeerConnectionState::kClosed, "closed" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::PeerConnectionState, PeerConnectionStateMap)
} // namespace node_webrtc