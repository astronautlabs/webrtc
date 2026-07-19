#include "webrtc_addon.h"
#include <webrtc/rtc_base/ssl_adapter.h>
#include <uv.h>

#include "src/interfaces/media_stream.h"
#include "src/interfaces/media_stream_track.h"
#include "src/interfaces/rtc_audio_sink.h"
#include "src/interfaces/rtc_audio_source.h"
#include "src/interfaces/rtc_data_channel.h"
#include "src/interfaces/rtc_dtls_transport.h"
#include "src/interfaces/rtc_ice_candidate.h"
#include "src/interfaces/rtc_ice_transport.h"
#include "src/interfaces/rtc_peer_connection.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/interfaces/rtc_rtp_receiver.h"
#include "src/interfaces/rtc_rtp_sender.h"
#include "src/interfaces/rtc_rtp_transceiver.h"
#include "src/interfaces/rtc_sctp_transport.h"
#include "src/interfaces/rtc_session_description.h"
#include "src/interfaces/rtc_video_sink.h"
#include "src/interfaces/rtc_video_source.h"
#include "src/methods/get_display_media.h"
#include "src/methods/get_user_media.h"
#include "src/methods/i420_helpers.h"
#include "src/node/async_context_releaser.h"
#include "src/node/error_factory.h"
#include "src/utilities/napi_ref_ptr.h"

#ifdef DEBUG
#include "src/test.h"
#endif

namespace node_webrtc {
    NODE_API_ADDON(Addon);

    Addon::Addon(Napi::Env env, Napi::Object exports):
        NapiRefTeardownIndicator(env),
        _env(env)
    {
        env.SetInstanceData(this);
        
        // Set up libwebrtc logging
        const char* debugVar = std::getenv("LIBWEBRTC_DEBUG");
        if (debugVar && std::string { debugVar } == "1") {
            webrtc::LogMessage::SetLogToStderr(true);
            webrtc::LogMessage::LogToDebug(webrtc::LoggingSeverity::LS_VERBOSE);
        } else {
            webrtc::LogMessage::SetLogToStderr(false);
        }
        
        // Clean up SSL when we're all done.
        env.AddCleanupHook([]() {
            webrtc::CleanupSSL();
        });

        // Register exports
        node_webrtc::AsyncContextReleaser::Init(env, exports);
        node_webrtc::ErrorFactory::Init(env, exports);
        node_webrtc::GetDisplayMedia::Init(env, exports);
        node_webrtc::GetUserMedia::Init(env, exports);
        node_webrtc::I420Helpers::Init(env, exports);
        node_webrtc::MediaStream::Init(env, exports);
        node_webrtc::MediaStreamTrack::Init(env, exports);
        node_webrtc::PeerConnectionFactory::Init(env, exports);
        node_webrtc::RTCAudioSink::Init(env, exports);
        node_webrtc::RTCAudioSource::Init(env, exports);
        node_webrtc::RTCDataChannel::Init(env, exports);
        node_webrtc::RTCIceCandidate::Init(env, exports);
        node_webrtc::RTCIceTransport::Init(env, exports);
        node_webrtc::RTCDtlsTransport::Init(env, exports);
        node_webrtc::RTCPeerConnection::Init(env, exports);
        node_webrtc::RTCSessionDescription::Init(env, exports);
        node_webrtc::RTCRtpReceiver::Init(env, exports);
        node_webrtc::RTCRtpSender::Init(env, exports);
        node_webrtc::RTCRtpTransceiver::Init(env, exports);
        node_webrtc::RTCSctpTransport::Init(env, exports);
        node_webrtc::RTCVideoSink::Init(env, exports);
        node_webrtc::RTCVideoSource::Init(env, exports);
#ifdef DEBUG
        node_webrtc::Test::Init(env, exports);
#endif
    }
} // namespace node_webrtc