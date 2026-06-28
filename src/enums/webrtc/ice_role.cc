#include "src/enums/webrtc/ice_role.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::IceRole, std::string> IceRoleMap {
        {
            { webrtc::IceRole::ICEROLE_CONTROLLING, "controlling" },
            { webrtc::IceRole::ICEROLE_CONTROLLED, "controlled" },
            { webrtc::IceRole::ICEROLE_UNKNOWN, "unknown" }, // unsupported
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::IceRole, IceRoleMap)
} // namespace node_webrtc

