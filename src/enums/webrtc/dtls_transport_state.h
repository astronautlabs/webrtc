#pragma once
#include "src/enums/enum_class_conversion.h"
#include <webrtc/api/dtls_transport_interface.h>

namespace node_webrtc {
    DECLARE_ENUM_CLASS_CONVERTER(webrtc::DtlsTransportState);
} // namespace node_webrtc
