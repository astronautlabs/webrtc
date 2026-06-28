#include "src/enums/node_webrtc/rtc_priority_type.h"

namespace node_webrtc {
    static const bidirectional_map<RTCPriorityType, std::string> RTCPriorityTypeMap {
        {
            { RTCPriorityType::kVeryLow, "very-low" },
            { RTCPriorityType::kLow, "low" },
            { RTCPriorityType::kMedium, "medium" },
            { RTCPriorityType::kHigh, "high" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(RTCPriorityType, RTCPriorityTypeMap)
} // namespace node_webrtc