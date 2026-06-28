#include "src/utilities/log.h"

#include <cstdlib>
#include <iostream>

namespace node_webrtc {
    bool hasConfiguredLogging = false;
    bool loggingEnabled = false;
    void Log(void* Owner, std::string message) {
        if (!hasConfiguredLogging) {
            hasConfiguredLogging = true;
            const char* logVar = std::getenv("NODE_WEBRTC_LOGGING");
            if (logVar && std::string { logVar } == "1") {
                loggingEnabled = true;
            }
        }

        if (loggingEnabled)
            std::cerr << "@/webrtc native [" << std::hex << reinterpret_cast<uint64_t>(Owner) << std::dec << "] " << message << std::endl;
    }
} // namespace node_webrtc