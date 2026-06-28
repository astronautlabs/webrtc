#pragma once
#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/peer_connection_interface.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::PeerConnectionInterface::IceConnectionState);
} // namespace node_webrtc