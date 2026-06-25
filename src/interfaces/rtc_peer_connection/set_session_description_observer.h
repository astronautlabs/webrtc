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

#include <src/api/set_local_description_observer_interface.h>
#include <src/api/set_remote_description_observer_interface.h>
#include <webrtc/api/jsep.h>

#include "src/interfaces/rtc_peer_connection.h"
#include "src/node/promise.h"

namespace webrtc { class RTCError; }

namespace node_webrtc {
	class SetSessionDescriptionObserver: 
		public PromiseCreator<RTCPeerConnection>, 
		public webrtc::SetLocalDescriptionObserverInterface,
		public webrtc::SetRemoteDescriptionObserverInterface {
	public:
		SetSessionDescriptionObserver(
			RTCPeerConnection* peer_connection,
			Napi::Promise::Deferred deferred)
			: PromiseCreator<RTCPeerConnection>(peer_connection, deferred) {
			pc = peer_connection;
		}

        void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;
        void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

		void OnSuccess();
		void OnFailure(webrtc::RTCError);

	private:
		RTCPeerConnection* pc;
	};

}  // namespace node_webrtc
