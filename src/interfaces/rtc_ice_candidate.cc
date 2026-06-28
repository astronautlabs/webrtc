#include "src/interfaces/rtc_ice_candidate.h"

namespace node_webrtc {

    Napi::FunctionReference& RTCIceCandidate::constructor() {
        static Napi::FunctionReference constructor;
        return constructor;
    }

    void RTCIceCandidate::Init(Napi::Env env, Napi::Object exports) {
        auto func = DefineClass(env, "RTCIceCandidate", { InstanceAccessor("candidate", &RTCIceCandidate::GetCandidate, nullptr), InstanceAccessor("sdpMid", &RTCIceCandidate::GetSdpMid, nullptr), InstanceAccessor("sdpMLineIndex", &RTCIceCandidate::GetSdpMLineIndex, nullptr), InstanceAccessor("usernameFragment", &RTCIceCandidate::GetUsernameFragment, nullptr), InstanceMethod("toJSON", &RTCIceCandidate::ToJSON) });

        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        exports.Set("RTCIceCandidate", func);
    }

    RTCIceCandidate::RTCIceCandidate(const Napi::CallbackInfo& info) :
        Napi::ObjectWrap<RTCIceCandidate>(info) {
        auto env = info.Env();

        if (info.Length() == 0 || !info[0].IsObject()) {
            Napi::TypeError::New(env, "Expected an RTCIceCandidateInit dictionary").ThrowAsJavaScriptException();
            return;
        }

        auto initDict = info[0].As<Napi::Object>();

        // 1. Parse 'candidate' (default is empty string in spec)
        auto candidateVal = initDict.Get("candidate");
        if (candidateVal.IsString()) {
            _candidate = candidateVal.As<Napi::String>().Utf8Value();
        } else {
            _candidate = "";
        }

        // 2. Parse 'sdpMid' (nullable)
        auto sdpMidVal = initDict.Get("sdpMid");
        if (sdpMidVal.IsString()) {
            _sdpMid = sdpMidVal.As<Napi::String>().Utf8Value();
        } else if (!sdpMidVal.IsNull() && !sdpMidVal.IsUndefined()) {
            Napi::TypeError::New(env, "'sdpMid' must be a string or null").ThrowAsJavaScriptException();
            return;
        }

        // 3. Parse 'sdpMLineIndex' (nullable)
        auto sdpMLineIndexVal = initDict.Get("sdpMLineIndex");
        if (sdpMLineIndexVal.IsNumber()) {
            _sdpMLineIndex = sdpMLineIndexVal.As<Napi::Number>().Int32Value();
        } else if (!sdpMLineIndexVal.IsNull() && !sdpMLineIndexVal.IsUndefined()) {
            Napi::TypeError::New(env, "'sdpMLineIndex' must be a number or null").ThrowAsJavaScriptException();
            return;
        }

        // 4. Parse 'usernameFragment' (nullable)
        auto usernameFragmentVal = initDict.Get("usernameFragment");
        if (usernameFragmentVal.IsString()) {
            _usernameFragment = usernameFragmentVal.As<Napi::String>().Utf8Value();
        } else if (!usernameFragmentVal.IsNull() && !usernameFragmentVal.IsUndefined()) {
            Napi::TypeError::New(env, "'usernameFragment' must be a string or null").ThrowAsJavaScriptException();
            return;
        }
    }

    Napi::Value RTCIceCandidate::GetCandidate(const Napi::CallbackInfo& info) {
        return Napi::String::New(info.Env(), _candidate);
    }

    Napi::Value RTCIceCandidate::GetSdpMid(const Napi::CallbackInfo& info) {
        if (_sdpMid.has_value()) {
            return Napi::String::New(info.Env(), _sdpMid.value());
        }
        return info.Env().Null();
    }

    Napi::Value RTCIceCandidate::GetSdpMLineIndex(const Napi::CallbackInfo& info) {
        if (_sdpMLineIndex.has_value()) {
            return Napi::Number::New(info.Env(), _sdpMLineIndex.value());
        }
        return info.Env().Null();
    }

    Napi::Value RTCIceCandidate::GetUsernameFragment(const Napi::CallbackInfo& info) {
        if (_usernameFragment.has_value()) {
            return Napi::String::New(info.Env(), _usernameFragment.value());
        }
        return info.Env().Null();
    }

    Napi::Value RTCIceCandidate::ToJSON(const Napi::CallbackInfo& info) {
        auto env = info.Env();
        auto result = Napi::Object::New(env);

        result.Set("candidate", Napi::String::New(env, _candidate));

        if (_sdpMid.has_value()) {
            result.Set("sdpMid", Napi::String::New(env, _sdpMid.value()));
        } else {
            result.Set("sdpMid", env.Null());
        }

        if (_sdpMLineIndex.has_value()) {
            result.Set("sdpMLineIndex", Napi::Number::New(env, _sdpMLineIndex.value()));
        } else {
            result.Set("sdpMLineIndex", env.Null());
        }

        if (_usernameFragment.has_value()) {
            result.Set("usernameFragment", Napi::String::New(env, _usernameFragment.value()));
        } else {
            result.Set("usernameFragment", env.Null());
        }

        return result;
    }

} // namespace node_webrtc