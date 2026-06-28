#include "src/enums/webrtc/ice_connection_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::IceConnectionState, std::string> IceConnectionStateMap {
        {
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew, "new" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionChecking, "checking" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed, "closed" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted, "completed" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected, "connected" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionDisconnected, "disconnected" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionFailed, "failed" },
            { webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionMax, "max" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::IceConnectionState, IceConnectionStateMap)
} // namespace node_webrtc