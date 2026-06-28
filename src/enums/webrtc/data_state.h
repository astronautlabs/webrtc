#pragma once
#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/data_channel_interface.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::DataChannelInterface::DataState);
} // namespace node_webrtc