#include "src/enums/node_webrtc/binary_type.h"

namespace node_webrtc {
    static const bidirectional_map<BinaryType, std::string> BinaryTypeMap {
        {
            { BinaryType::kBlob, "blob" },
            { BinaryType::kArrayBuffer, "arraybuffer" }
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(BinaryType, BinaryTypeMap)
}
