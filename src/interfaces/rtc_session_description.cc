#include "src/interfaces/rtc_session_description.h"

namespace node_webrtc {

Napi::FunctionReference& RTCSessionDescription::constructor() {
  static Napi::FunctionReference constructor;
  return constructor;
}

void RTCSessionDescription::Init(Napi::Env env, Napi::Object exports) {
  auto func = DefineClass(env, "RTCSessionDescription", {
    InstanceAccessor("type", &RTCSessionDescription::GetType, nullptr),
    InstanceAccessor("sdp", &RTCSessionDescription::GetSdp, nullptr),
    InstanceMethod("toJSON", &RTCSessionDescription::ToJSON)
  });

  constructor() = Napi::Persistent(func);
  constructor().SuppressDestruct();

  exports.Set("RTCSessionDescription", func);
}

RTCSessionDescription::RTCSessionDescription(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<RTCSessionDescription>(info) {
  auto env = info.Env();

  // The WebRTC spec expects an RTCSessionDescriptionInit dictionary
  if (info.Length() > 0 && info[0].IsObject()) {
    auto initDict = info[0].As<Napi::Object>();
    
    auto typeVal = initDict.Get("type");
    if (typeVal.IsString()) {
      _type = typeVal.As<Napi::String>().Utf8Value();
    }

    auto sdpVal = initDict.Get("sdp");
    if (sdpVal.IsString()) {
      _sdp = sdpVal.As<Napi::String>().Utf8Value();
    }
  } else if (info.Length() > 0 && !info[0].IsUndefined() && !info[0].IsNull()) {
    // If they passed something that isn't an object (and isn't null/undefined)
    Napi::TypeError::New(env, "Expected an RTCSessionDescriptionInit dictionary")
        .ThrowAsJavaScriptException();
  }
}

Napi::Value RTCSessionDescription::GetType(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), _type);
}

Napi::Value RTCSessionDescription::GetSdp(const Napi::CallbackInfo& info) {
  return Napi::String::New(info.Env(), _sdp);
}

Napi::Value RTCSessionDescription::ToJSON(const Napi::CallbackInfo& info) {
  auto env = info.Env();
  auto result = Napi::Object::New(env);
  
  // toJSON() allows easy serialization during signaling (e.g. JSON.stringify(pc.localDescription))
  result.Set("type", Napi::String::New(env, _type));
  result.Set("sdp", Napi::String::New(env, _sdp));
  
  return result;
}

}  // namespace node_webrtc