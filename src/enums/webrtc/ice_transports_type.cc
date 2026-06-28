#include "src/enums/webrtc/ice_transports_type.h"
namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::IceTransportsType, std::string> IceTransportsTypeMap {
        {
        { webrtc::PeerConnectionInterface::IceTransportsType::kAll, "all" },
        { webrtc::PeerConnectionInterface::IceTransportsType::kRelay, "relay" },
        { webrtc::PeerConnectionInterface::IceTransportsType::kNoHost, "no-host" },
        { webrtc::PeerConnectionInterface::IceTransportsType::kNone, "none"  },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::IceTransportsType, IceTransportsTypeMap)
} // namespace node_webrtc