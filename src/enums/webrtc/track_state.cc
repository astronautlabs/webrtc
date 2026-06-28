#include "src/enums/webrtc/track_state.h"

namespace node_webrtc {
    static const bidirectional_map<webrtc::MediaStreamTrackInterface::TrackState, std::string> TrackStateMap {
        {
        { webrtc::MediaStreamTrackInterface::TrackState::kEnded, "ended" },
        { webrtc::MediaStreamTrackInterface::TrackState::kLive, "live" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::MediaStreamTrackInterface::TrackState, TrackStateMap)
} // namespace node_webrtc
