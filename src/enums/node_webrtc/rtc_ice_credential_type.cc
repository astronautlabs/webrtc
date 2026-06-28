#include "src/enums/node_webrtc/rtc_ice_credential_type.h"

namespace node_webrtc {
    static const bidirectional_map<RTCIceCredentialType, std::string> RTCIceCredentialTypeMap {
        {
            { RTCIceCredentialType::kPassword, "password" },
            { RTCIceCredentialType::kOAuth, "oauth" }
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(RTCIceCredentialType, RTCIceCredentialTypeMap)
} // namespace node_webrtc