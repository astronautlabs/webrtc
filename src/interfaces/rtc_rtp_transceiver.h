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

#include "src/utilities/napi_ref_ptr.h"
#include "src/converters/napi.h"
#include "src/converters/napi.h"
#include "src/node/async_object_wrap.h"
#include "src/node/wrap.h"
#include "src/interfaces/rtc_peer_connection.h"

namespace webrtc { class RtpTransceiverInterface; }

namespace node_webrtc {
	class RTCPeerConnection;
	class RTCRtpSender;
	class RTCRtpReceiver;
	class RTCRtpTransceiver : public Napi::ObjectWrap<RTCRtpTransceiver> {
	public:
		explicit RTCRtpTransceiver(const Napi::CallbackInfo&);
		static void Init(Napi::Env, Napi::Object);
		static Napi::FunctionReference& constructor();
		void Finalize(Napi::Env env) override;
		static RTCRtpTransceiver* Create(RTCPeerConnection* pc, RTCRtpSender* sender, RTCRtpReceiver* receiver);
		void updateMembers(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtcTransceiver);

		inline RTCRtpReceiver* getReceiver() { return this->receiver;  }
		inline uintptr_t getId() { return id;  }
		inline void setId(uintptr_t value) { id = value; }
	private:

		inline std::string directionToString(webrtc::RtpTransceiverDirection rtcDirection) {
			if (rtcDirection == webrtc::RtpTransceiverDirection::kInactive)
				return "inactive";
			else if (rtcDirection == webrtc::RtpTransceiverDirection::kRecvOnly)
				return "recvonly";
			else if (rtcDirection == webrtc::RtpTransceiverDirection::kSendOnly)
				return "sendonly";
			else if (rtcDirection == webrtc::RtpTransceiverDirection::kSendRecv)
				return "sendrecv";
			return "unknown";
		}

		uintptr_t id;
		std::string mid = "";
		RTCPeerConnection* pc = nullptr;
		RTCRtpSender* sender = nullptr;
		RTCRtpReceiver* receiver = nullptr;
		bool stopped = false;
		std::string direction = "inactive";
		std::string currentDirection = "inactive";
		std::string firedDirection = "inactive";

		Napi::Reference<Napi::Value> pcRef;
		Napi::Reference<Napi::Value> senderRef;
		Napi::Reference<Napi::Value> receiverRef;

		Napi::Value GetMid(const Napi::CallbackInfo&);
		Napi::Value GetSender(const Napi::CallbackInfo&);
		Napi::Value GetReceiver(const Napi::CallbackInfo&);
		Napi::Value GetStopped(const Napi::CallbackInfo&);
		Napi::Value GetDirection(const Napi::CallbackInfo&);
		void SetDirection(const Napi::CallbackInfo&, const Napi::Value&);
		Napi::Value GetCurrentDirection(const Napi::CallbackInfo&);

		Napi::Value Stop(const Napi::CallbackInfo&);
		Napi::Value SetCodecPreferences(const Napi::CallbackInfo&);
	};

	DECLARE_TO_NAPI(RTCRtpTransceiver*)

}
