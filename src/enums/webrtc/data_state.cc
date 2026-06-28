#include "src/enums/webrtc/data_state.h"
#include <src/api/data_channel_interface.h>

namespace node_webrtc {
    static const bidirectional_map<webrtc::DataChannelInterface::DataState, std::string> DataStateMap {
        {
            { webrtc::DataChannelInterface::DataState::kClosed, "closed" },
            { webrtc::DataChannelInterface::DataState::kClosing, "closing" },
            { webrtc::DataChannelInterface::DataState::kConnecting, "connecting" },
            { webrtc::DataChannelInterface::DataState::kOpen, "open" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::DataChannelInterface::DataState, DataStateMap)
} // namespace node_webrtc