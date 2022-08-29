#!/bin/bash

set -e

set -v

export PATH=$DEPOT_TOOLS:$PATH

export TARGETS="webrtc libjingle_peerconnection"
if [[ "$TARGET_ARCH" == arm* ]]; then
  export TARGETS="$TARGETS pc:peerconnection libc++ libc++abi"
fi

if [ -z "$PARALLELISM" ]; then
  PARALLELISM=24
fi

ninja -j $PARALLELISM
