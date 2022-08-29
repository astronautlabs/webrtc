/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <memory>

#include <node-addon-api/napi.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/converters/napi.h"
#include "src/node/async_object_wrap.h"
#include "src/node/wrap.h"
#include "src/interfaces/media_stream_track.h"
#include "src/interfaces/media_stream.h"
#include "src/interfaces/rtc_peer_connection.h"
#include "src/interfaces/rtc_dtls_transport.h"
#include "src/utilities/napi_ref_ptr.h"

namespace webrtc { class RtpReceiverInterface; }

namespace node_webrtc {
	class RTCRtpTransceiver;
	class RTCPeerConnection;

	class RTCRtpReceiver : public Napi::ObjectWrap<RTCRtpReceiver> {
	public:
		explicit RTCRtpReceiver(const Napi::CallbackInfo&);
		static void Init(Napi::Env, Napi::Object);

		inline void setTransport(RTCDtlsTransport* transport) {
			this->transport = transport;
		}

		inline void setSources(std::vector<webrtc::RtpSource> value) {
			sources = value;
		}

		inline void setTransceiver(RTCRtpTransceiver* value) {
			transceiver = value;
		}

		inline std::string getId() {
			return id;
		}

		inline void setId(std::string value) {
			id = value;
		}

		static inline void setVideoCapabilities(webrtc::RtpCapabilities caps) {
			videoCapabilities = caps;
		}

		static inline void setAudioCapabilities(webrtc::RtpCapabilities caps) {
			audioCapabilities = caps;
		}

		static Napi::FunctionReference& constructor();
		static RTCRtpReceiver* Create(
			RTCPeerConnection* pc,
			MediaStreamTrack* track,
			std::vector<MediaStream*> streams
		);
	private:
		static webrtc::RtpCapabilities audioCapabilities;
		static webrtc::RtpCapabilities videoCapabilities;

		Napi::Value GetTrack(const Napi::CallbackInfo&);
		Napi::Value GetTransport(const Napi::CallbackInfo&);
		Napi::Value GetRtcpTransport(const Napi::CallbackInfo&);
		static Napi::Value GetCapabilities(const Napi::CallbackInfo&);
		Napi::Value GetParameters(const Napi::CallbackInfo&);
		Napi::Value GetContributingSources(const Napi::CallbackInfo&);
		Napi::Value GetSynchronizationSources(const Napi::CallbackInfo&);
		Napi::Value GetStats(const Napi::CallbackInfo&);

		rtc::scoped_refptr<webrtc::RtpReceiverInterface> _receiver;

		std::string id;
		napi_ref_ptr<RTCRtpTransceiver> transceiver;
		napi_ref_ptr<RTCPeerConnection> pc;
		napi_ref_ptr<MediaStreamTrack> track;
		napi_ref_ptr<RTCDtlsTransport> transport;
		std::vector<napi_ref_ptr<MediaStream>> streams;
		webrtc::RtpParameters parameters;
		std::vector<webrtc::RtpSource> sources;

		Napi::Reference<Napi::Value> pcRef;
		Napi::Reference<Napi::Value> trackRef;

	};

	DECLARE_TO_AND_FROM_NAPI(RTCRtpReceiver*)

}  // namespace node_webrtc
