#include "src/enums/webrtc/rtp_transceiver_direction.h"

#include <webrtc/api/rtp_transceiver_interface.h>  // IWYU pragma: keep
namespace node_webrtc {
    static const bidirectional_map<webrtc::RtpTransceiverDirection, std::string> RtpTransceiverDirectionMap {
        {
        { webrtc::RtpTransceiverDirection::kSendRecv, "sendrecv" },
        { webrtc::RtpTransceiverDirection::kSendOnly, "sendonly" },
        { webrtc::RtpTransceiverDirection::kRecvOnly, "recvonly" },
        { webrtc::RtpTransceiverDirection::kInactive, "inactive" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::RtpTransceiverDirection, RtpTransceiverDirectionMap)
} // namespace node_webrtc