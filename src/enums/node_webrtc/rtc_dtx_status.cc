#include "src/enums/node_webrtc/rtc_dtx_status.h"

namespace node_webrtc {
    static const bidirectional_map<RTCDtxStatus, std::string> RTCDtxStatusMap {
        {
            { RTCDtxStatus::kDisabled, "disabled" },
            { RTCDtxStatus::kEnabled, "enabled" }
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(RTCDtxStatus, RTCDtxStatusMap)
} // namespace node_webrtc