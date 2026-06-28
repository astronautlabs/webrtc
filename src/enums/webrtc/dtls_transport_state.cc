#include "src/enums/webrtc/dtls_transport_state.h"
#include <src/api/peer_connection_interface.h>

namespace node_webrtc {
    static const bidirectional_map<webrtc::DtlsTransportState, std::string> DtlsTransportStateMap {
        {
            { webrtc::DtlsTransportState::kNew, "new" },
            { webrtc::DtlsTransportState::kConnecting, "connecting" },
            { webrtc::DtlsTransportState::kConnected, "connected" },
            { webrtc::DtlsTransportState::kClosed, "closed" },
            { webrtc::DtlsTransportState::kFailed, "failed" },
            { webrtc::DtlsTransportState::kNumValues, "num-values" }, // unsupported
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::DtlsTransportState, DtlsTransportStateMap)
} // namespace node_webrtc
