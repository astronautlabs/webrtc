#include "src/enums/webrtc/degradation_preference.h"
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/rtp_parameters.h>

namespace node_webrtc {
    static const bidirectional_map<webrtc::DegradationPreference, std::string> DegradationPreferenceMap {
        {
            { webrtc::DegradationPreference::DISABLED, "disabled" }, // unsupported
            { webrtc::DegradationPreference::MAINTAIN_FRAMERATE, "maintain-framerate" },
            { webrtc::DegradationPreference::MAINTAIN_RESOLUTION, "maintain-resolution" },
            { webrtc::DegradationPreference::BALANCED, "balanced" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::DegradationPreference, DegradationPreferenceMap)
} // namespace node_webrtc
