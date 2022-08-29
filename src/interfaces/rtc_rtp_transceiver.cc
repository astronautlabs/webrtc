/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 * 
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_rtp_transceiver.h"

#include <webrtc/api/rtp_transceiver_interface.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/converters/absl.h"
#include "src/converters/arguments.h"
#include "src/converters/interfaces.h"
#include "src/dictionaries/webrtc/rtc_error.h"
#include "src/dictionaries/webrtc/rtp_encoding_parameters.h"
#include "src/dictionaries/webrtc/rtp_codec_capability.h"
#include "src/enums/webrtc/rtp_transceiver_direction.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/interfaces/rtc_rtp_receiver.h"
#include "src/interfaces/rtc_rtp_sender.h"

namespace node_webrtc {
	Napi::FunctionReference& RTCRtpTransceiver::constructor() {
		static Napi::FunctionReference constructor;
		return constructor;
	}

	void RTCRtpTransceiver::Finalize(Napi::Env env)
	{
		pcRef.Unref();
		senderRef.Unref();
		receiverRef.Unref();
	}

	RTCRtpTransceiver::RTCRtpTransceiver(const Napi::CallbackInfo& info) :
		Napi::ObjectWrap<RTCRtpTransceiver>(info)
	{
		if (info.Length() != 3 || !info[0].IsExternal() || !info[1].IsExternal() || !info[2].IsExternal()) {
			Napi::TypeError::New(info.Env(), "You cannot construct a RTCRtpTransceiver").ThrowAsJavaScriptException();
			return;
		}

		pc = info[0].As<Napi::External<RTCPeerConnection>>().Data();
		pcRef = Napi::Reference<Napi::Value>::New(pc->Value(), 1);
		sender = info[1].As<Napi::External<RTCRtpSender>>().Data();
		senderRef = Napi::Reference<Napi::Value>::New(sender->Value(), 1);
		receiver = info[2].As<Napi::External<RTCRtpReceiver>>().Data();
		receiverRef = Napi::Reference<Napi::Value>::New(receiver->Value(), 1);
	}

	Napi::Value RTCRtpTransceiver::GetMid(const Napi::CallbackInfo& info) {
		return Napi::String::New(info.Env(), mid);
	}

	Napi::Value RTCRtpTransceiver::GetSender(const Napi::CallbackInfo& info) {
		return this->sender->Value();
	}

	Napi::Value RTCRtpTransceiver::GetReceiver(const Napi::CallbackInfo& info) {
		return this->receiver->Value();
	}

	Napi::Value RTCRtpTransceiver::GetStopped(const Napi::CallbackInfo& info) {
		return Napi::Boolean::New(info.Env(), this->stopped);
	}

	Napi::Value RTCRtpTransceiver::GetDirection(const Napi::CallbackInfo& info) {
		return Napi::String::New(info.Env(), this->direction);
	}

	void RTCRtpTransceiver::SetDirection(const Napi::CallbackInfo& info, const Napi::Value& value) {
		auto maybeDirection = From<webrtc::RtpTransceiverDirection>(value);
		if (maybeDirection.IsInvalid()) {
			Napi::TypeError::New(info.Env(), maybeDirection.ToErrors()[0]).ThrowAsJavaScriptException();
			return;
		}
		pc->getUnderlying(this)->SetDirection(maybeDirection.UnsafeFromValid());
	}

	Napi::Value RTCRtpTransceiver::GetCurrentDirection(const Napi::CallbackInfo& info) {
		return Napi::String::New(info.Env(), currentDirection);
	}

	Napi::Value RTCRtpTransceiver::Stop(const Napi::CallbackInfo& info) {
		pc->getUnderlying(this)->Stop();
		return info.Env().Undefined();
	}

	Napi::Value RTCRtpTransceiver::SetCodecPreferences(const Napi::CallbackInfo& info) {
		CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, codecs, std::vector<webrtc::RtpCodecCapability>)
			auto capabilities = rtc::ArrayView<webrtc::RtpCodecCapability>(codecs);
		auto error = pc->getUnderlying(this)->SetCodecPreferences(capabilities);
		if (!error.ok()) {
			CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), &error, result, Napi::Value)
				Napi::Error(info.Env(), result).ThrowAsJavaScriptException();
		}
		return info.Env().Undefined();
	}

	RTCRtpTransceiver* RTCRtpTransceiver::Create(RTCPeerConnection* pc, RTCRtpSender* sender, RTCRtpReceiver* receiver)
	{
		auto env = constructor().Env();
		Napi::HandleScope scope(env);
		auto unwrapped = Unwrap(constructor().New({
			Napi::External<RTCPeerConnection>::New(env, pc),
			Napi::External<RTCRtpSender>::New(env, sender),
			Napi::External<RTCRtpReceiver>::New(env, receiver)
		}));

		unwrapped->Ref(); // Owned by RTCPeerConnection
		return unwrapped;
	}

	void RTCRtpTransceiver::updateMembers(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> rtcTransceiver)
	{
		if (currentDirection == "stopped")
			return;

		mid = rtcTransceiver->mid().has_value() ? rtcTransceiver->mid().value() : "";
		direction = directionToString(rtcTransceiver->direction());
		currentDirection = rtcTransceiver->current_direction().has_value() 
			? directionToString(rtcTransceiver->current_direction().value()) 
			: "";
		firedDirection = rtcTransceiver->fired_direction().has_value()
			? directionToString(rtcTransceiver->fired_direction().value())
			: "";
	}

	void RTCRtpTransceiver::Init(Napi::Env env, Napi::Object exports) {
		auto func = DefineClass(env, "RTCRtpTransceiver", {
			InstanceAccessor("mid", &RTCRtpTransceiver::GetMid, nullptr),
			InstanceAccessor("sender", &RTCRtpTransceiver::GetSender, nullptr),
			InstanceAccessor("receiver", &RTCRtpTransceiver::GetReceiver, nullptr),
			InstanceAccessor("stopped", &RTCRtpTransceiver::GetStopped, nullptr),
			InstanceAccessor("direction", &RTCRtpTransceiver::GetDirection, &RTCRtpTransceiver::SetDirection),
			InstanceAccessor("currentDirection", &RTCRtpTransceiver::GetCurrentDirection, nullptr),
			InstanceMethod("stop", &RTCRtpTransceiver::Stop),
			InstanceMethod("setCodecPreferences", &RTCRtpTransceiver::SetCodecPreferences),
		});

		constructor() = Napi::Persistent(func);
		constructor().SuppressDestruct();

		exports.Set("RTCRtpTransceiver", func);
	}

	TO_NAPI_IMPL(RTCRtpTransceiver*, pair) {
		return Pure(pair.second->Value().As<Napi::Value>());
	}
}
