/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_audio_source.h"

#include <webrtc/api/peer_connection_interface.h>
#include <webrtc/rtc_base/ref_counted_object.h>

#include "src/converters.h"
#include "src/converters/arguments.h"
#include "src/functional/maybe.h"
#include "src/interfaces/media_stream_track.h"

namespace node_webrtc {

Napi::FunctionReference& RTCAudioSource::constructor() {
  static Napi::FunctionReference constructor;
  return constructor;
}

RTCAudioSource::RTCAudioSource(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<RTCAudioSource>(info) {
  _source = new rtc::RefCountedObject<RTCAudioTrackSource>();
}

void RTCAudioSource::Finalize(Napi::Env env)
{
    // These are the tracks created via CreateTrack().
    // We will unref them so that they are collectable, 
    // but note that if they were added to one or more PeerConnections
    // via addTrack(), that they will still be referenced by those 
    // PeerConnections and thus will continue to remain uncollectable.

    for (auto track : _tracks)
        track->Unref();
}

Napi::Value RTCAudioSource::CreateTrack(const Napi::CallbackInfo&) {
  // TODO(mroberts): Again, we have some implicit factory we are threading around. How to handle?
  auto factory = PeerConnectionFactory::GetOrCreateDefault();
  auto track = factory->factory()->CreateAudioTrack(rtc::CreateRandomUuid(), _source);

  // Here the default reference will be owned by the RTCAudioSource. 
  // See RTCPeerConnection::AddTrack() for corresponding referencing logic in that case.

  auto wrappedTrack = MediaStreamTrack::wrap()->GetOrCreate(factory, track);
  _tracks.insert(wrappedTrack);
  return wrappedTrack->Value();
}

Napi::Value RTCAudioSource::OnData(const Napi::CallbackInfo& info) {
  CONVERT_ARGS_OR_THROW_AND_RETURN_NAPI(info, dict, RTCOnDataEventDict)
  _source->PushData(dict);
  return info.Env().Undefined();
}

void RTCAudioSource::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "RTCAudioSource", {
    InstanceMethod("createTrack", &RTCAudioSource::CreateTrack),
    InstanceMethod("onData", &RTCAudioSource::OnData)
  });

  constructor() = Napi::Persistent(func);
  constructor().SuppressDestruct();

  exports.Set("RTCAudioSource", func);
}

}  // namespace node_webrtc
