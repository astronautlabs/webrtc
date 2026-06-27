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

#include <string>

#include <node-addon-api/napi.h>

namespace node_webrtc {

class RTCSessionDescription : public Napi::ObjectWrap<RTCSessionDescription> {
 public:
  static void Init(Napi::Env env, Napi::Object exports);
  explicit RTCSessionDescription(const Napi::CallbackInfo& info);

  // JS Accessors and Methods
  Napi::Value GetType(const Napi::CallbackInfo& info);
  Napi::Value GetSdp(const Napi::CallbackInfo& info);
  Napi::Value ToJSON(const Napi::CallbackInfo& info);

  // C++ accessors for internal use (e.g., inside RTCPeerConnection::SetLocalDescription)
  const std::string& type() const { return _type; }
  const std::string& sdp() const { return _sdp; }

 private:
  // Using the static reference pattern to prevent teardown crashes
  static Napi::FunctionReference& constructor();

  std::string _type;
  std::string _sdp;
};

}  // namespace node_webrtc