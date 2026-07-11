#!/bin/bash

echo ==========================================================
echo Building libwebrtc
echo ==========================================================

set -e
set -v

export PATH=$DEPOT_TOOLS:$PATH

export TARGETS="webrtc libjingle_peerconnection builtin_video_encoder_factory builtin_video_decoder_factory media_engine"
if [[ "$TARGET_ARCH" == arm* ]]; then
  export TARGETS="$TARGETS pc:peerconnection libc++ libc++abi"
fi

if [ -z "$PARALLELISM" ]; then
  PARALLELISM=24
fi

ninja $TARGETS -j $PARALLELISM
