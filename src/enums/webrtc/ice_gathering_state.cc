#include "src/enums/webrtc/ice_gathering_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::IceGatheringState, std::string> IceGatheringStateMap {
        {
            { webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew, "new" },
            { webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringGathering, "gathering" },
            { webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete, "complete" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::IceGatheringState, IceGatheringStateMap)
} // namespace node_webrtc