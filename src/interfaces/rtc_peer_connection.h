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

#include <vector>

#include <node-addon-api/napi.h>
#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/node/async_object_wrap_with_loop.h"
#include "src/dictionaries/node_webrtc/extended_rtc_configuration.h"
#include "src/dictionaries/node_webrtc/rtc_session_description_init.h"
#include "src/interfaces/rtc_rtp_transceiver.h"
#include "src/interfaces/rtc_rtp_receiver.h"
#include "src/interfaces/rtc_rtp_sender.h"
#include "src/interfaces/rtc_sctp_transport.h"
#include "src/interfaces/media_stream_track.h"
#include "src/interfaces/media_stream.h"
#include "src/interfaces/rtc_ice_transport.h"

namespace webrtc {

	class DataChannelInterface;
	class IceCandidateInterface;
	class MediaStreamInterface;
	class RtpReceiverInterface;
	class RtpTransceiverInterface;

}  // namespace webrtc

namespace node_webrtc {

	class RTCDataChannel;
	class PeerConnectionFactory;

	class RTCPeerConnection
		: public AsyncObjectWrapWithLoop<RTCPeerConnection>
		, public webrtc::PeerConnectionObserver {
	public:
		explicit RTCPeerConnection(const Napi::CallbackInfo&);

		void Finalize(Napi::Env env) override;

		//
		// PeerConnectionObserver implementation.
		//
		void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;
		void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
		void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
		void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
		void OnIceCandidateError(const std::string& host_candidate, const std::string& url, int error_code, const std::string& error_text) override;
		void OnRenegotiationNeeded() override;

		void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

		void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
		void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

		void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
			const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
		void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

		static void Init(Napi::Env, Napi::Object);

		static Napi::FunctionReference& constructor();

		void SaveLastSdp(const RTCSessionDescriptionInit& lastSdp);

		void onSetDescriptionComplete();

		rtc::scoped_refptr<webrtc::RtpSenderInterface> getUnderlying(RTCRtpSender* sender);
		rtc::scoped_refptr<webrtc::RtpReceiverInterface> getUnderlying(RTCRtpReceiver* receiver);
		rtc::scoped_refptr<webrtc::RtpTransceiverInterface> getUnderlying(RTCRtpTransceiver* transceiver);
		inline bool isClosed() {
			return !_factory || !_jinglePeerConnection;
		}
		
	private:
		void processStateChangesPlanB();
		void processStateChangesUnifiedPlan();
		bool validateConfiguration(webrtc::PeerConnectionInterface::RTCConfiguration configuration);
		
		inline bool isPlanB() { 
			return _jinglePeerConnection 
				? (_jinglePeerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kPlanB) 
				: false; 
		}

		inline bool isUnifiedPlan() { 
			return _jinglePeerConnection
				? (_jinglePeerConnection->GetConfiguration().sdp_semantics == webrtc::SdpSemantics::kUnifiedPlan)
				: false;
		}

		Napi::Value AddTrack(const Napi::CallbackInfo&);
		Napi::Value AddTransceiver(const Napi::CallbackInfo&);
		Napi::Value RemoveTrack(const Napi::CallbackInfo&);
		Napi::Value CreateOffer(const Napi::CallbackInfo&);
		Napi::Value CreateAnswer(const Napi::CallbackInfo&);
		Napi::Value SetLocalDescription(const Napi::CallbackInfo&);
		Napi::Value SetRemoteDescription(const Napi::CallbackInfo&);
		Napi::Value UpdateIce(const Napi::CallbackInfo&);
		Napi::Value AddIceCandidate(const Napi::CallbackInfo&);
		Napi::Value CreateDataChannel(const Napi::CallbackInfo&);
		/*
		Napi::Value GetLocalStreams(const Napi::CallbackInfo&);
		Napi::Value GetRemoteStreams(const Napi::CallbackInfo&);
		Napi::Value GetStreamById(const Napi::CallbackInfo&);
		Napi::Value AddStream(const Napi::CallbackInfo&);
		Napi::Value RemoveStream(const Napi::CallbackInfo&);
		*/
		Napi::Value GetConfiguration(const Napi::CallbackInfo&);
		Napi::Value SetConfiguration(const Napi::CallbackInfo&);
		Napi::Value GetReceivers(const Napi::CallbackInfo&);
		Napi::Value GetSenders(const Napi::CallbackInfo&);
		Napi::Value GetStats(const Napi::CallbackInfo&);
		Napi::Value LegacyGetStats(const Napi::CallbackInfo&);
		Napi::Value GetTransceivers(const Napi::CallbackInfo&);
		Napi::Value Close(const Napi::CallbackInfo&);
		Napi::Value RestartIce(const Napi::CallbackInfo&);

		Napi::Value GetCanTrickleIceCandidates(const Napi::CallbackInfo&);
		Napi::Value GetConnectionState(const Napi::CallbackInfo&);
		Napi::Value GetCurrentLocalDescription(const Napi::CallbackInfo&);
		Napi::Value GetLocalDescription(const Napi::CallbackInfo&);
		Napi::Value GetPendingLocalDescription(const Napi::CallbackInfo&);
		Napi::Value GetCurrentRemoteDescription(const Napi::CallbackInfo&);
		Napi::Value GetRemoteDescription(const Napi::CallbackInfo&);
		Napi::Value GetPendingRemoteDescription(const Napi::CallbackInfo&);
		Napi::Value GetIceConnectionState(const Napi::CallbackInfo&);
		Napi::Value GetSctp(const Napi::CallbackInfo&);
		Napi::Value GetSignalingState(const Napi::CallbackInfo&);
		Napi::Value GetIceGatheringState(const Napi::CallbackInfo&);

		RTCSessionDescriptionInit _lastSdp;

		UnsignedShortRange _port_range;
		ExtendedRTCConfiguration _cached_configuration;
		rtc::scoped_refptr<webrtc::PeerConnectionInterface> _jinglePeerConnection;

		PeerConnectionFactory* _factory = nullptr;
		bool _shouldReleaseFactory = false;

		std::set<RTCDataChannel*> _channels;
		std::set<RTCDataChannel*> _peerChannels;
		std::map<uintptr_t, RTCRtpTransceiver*> _transceivers;
		std::map<std::string, RTCRtpReceiver*> _receivers;
		std::map<std::string, RTCRtpSender*> _senders;
		std::map<std::string, MediaStreamTrack*> _tracks;
		std::map<void*, RTCDtlsTransport*> _dtlsTransports;
		std::map<void*, RTCIceTransport*> _iceTransports;
		/**
		 * A set of tracks which are referenced by this RTCPeerConnection, but are not
		 * owned by it. These may be owned by an RTCAudioSource/RTCVideoSource or by another
		 * RTCPeerConnection. We keep a list of them so that we can unreference them when the
		 * RTCPeerConnection closes.
		 */
		std::map<std::string, MediaStreamTrack*> _externalTracks;

		RTCRtpTransceiver* createOrUpdateTransceiver(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtpTransceiver);
		RTCRtpReceiver* createOrUpdateReceiver(rtc::scoped_refptr<webrtc::RtpReceiverInterface> rtpReceiver);
		RTCRtpSender* createOrUpdateSender(rtc::scoped_refptr<webrtc::RtpSenderInterface> rtpSender, std::string kind);
		MediaStreamTrack* getTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> rtpTrack);
		MediaStreamTrack* getKnownTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> rtpTrack);
		bool isOwned(MediaStreamTrack* track);
		bool isOwned(RTCRtpSender* sender);


		std::set<MediaStream*> _streams;
		RTCSctpTransport* _sctpTransport = nullptr;
	};

}  // namespace node_webrtc
