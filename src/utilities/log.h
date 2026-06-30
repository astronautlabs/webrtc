#pragma once

#include "src/utilities/nameof.h"
#include <iostream>
#include <string>

namespace node_webrtc {
    extern bool hasConfiguredLogging;
    extern bool loggingEnabled;

    template <typename T>
    void Log(std::string message) {
        if (!hasConfiguredLogging) {
            hasConfiguredLogging = true;
            const char* logVar = std::getenv("NODE_WEBRTC_LOGGING");
            if (logVar && std::string { logVar } == "1") {
                loggingEnabled = true;
            }
        }

        if (loggingEnabled)
            std::cerr 
                << std::string { NAMEOF_TYPE(T) } << " -> " 
                << message 
                << std::endl;
    }

    template <typename T>
    void Log(T* Owner, std::string message) {
        if (!hasConfiguredLogging) {
            hasConfiguredLogging = true;
            const char* logVar = std::getenv("NODE_WEBRTC_LOGGING");
            if (logVar && std::string { logVar } == "1") {
                loggingEnabled = true;
            }
        }

        if (loggingEnabled)
            std::cerr 
                << std::string { NAMEOF_TYPE(T) } << ":"
                    << std::hex << reinterpret_cast<uint64_t>(Owner) << std::dec 
                << " -> "
                << message 
                << std::endl;

    }
} // namespace node_webrtc