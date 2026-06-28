#include "src/enums/webrtc/signaling_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::SignalingState, std::string> SignalingStateMap {
        {
        { webrtc::PeerConnectionInterface::SignalingState::kStable, "stable" },
        { webrtc::PeerConnectionInterface::SignalingState::kHaveLocalOffer, "have-local-offer" },
        { webrtc::PeerConnectionInterface::SignalingState::kHaveRemoteOffer, "have-remote-offer" },
        { webrtc::PeerConnectionInterface::SignalingState::kHaveLocalPrAnswer, "have-local-pranswer" },
        { webrtc::PeerConnectionInterface::SignalingState::kHaveRemotePrAnswer, "have-remote-pranswer" },
        { webrtc::PeerConnectionInterface::SignalingState::kClosed, "closed" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::SignalingState, SignalingStateMap)
} // namespace node_webrtc