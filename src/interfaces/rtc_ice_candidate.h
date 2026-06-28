#pragma once

#include <string>
#include <optional>
#include <node-addon-api/napi.h>

namespace node_webrtc {

class RTCIceCandidate : public Napi::ObjectWrap<RTCIceCandidate> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  explicit RTCIceCandidate(const Napi::CallbackInfo& info);

  // JS Accessors and Methods
  Napi::Value GetCandidate(const Napi::CallbackInfo& info);
  Napi::Value GetSdpMid(const Napi::CallbackInfo& info);
  Napi::Value GetSdpMLineIndex(const Napi::CallbackInfo& info);
  Napi::Value GetUsernameFragment(const Napi::CallbackInfo& info);
  Napi::Value ToJSON(const Napi::CallbackInfo& info);

  // C++ accessors for internal use (e.g., inside RTCPeerConnection::AddIceCandidate)
  const std::string& candidate() const { return _candidate; }
  const std::optional<std::string>& sdpMid() const { return _sdpMid; }
  const std::optional<int32_t>& sdpMLineIndex() const { return _sdpMLineIndex; }
  const std::optional<std::string>& usernameFragment() const { return _usernameFragment; }

 private:
  static Napi::FunctionReference& constructor();

  std::string _candidate;
  std::optional<std::string> _sdpMid;
  std::optional<int32_t> _sdpMLineIndex;
  std::optional<std::string> _usernameFragment;
};

}  // namespace node_webrtc