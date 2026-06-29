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
#include <type_traits>

#include <node-addon-api/napi.h>

#include "src/utilities/napi_ref_ptr.h"
#include "utility.h"

namespace node_webrtc {
    template <typename T>
    class PromiseCreator {
    public:
        PromiseCreator(
            napi_ref_ptr<T> target,
            Napi::Promise::Deferred deferred
        ) :
            _target(target),
            _deferred(deferred) 
        { 
        }

        template <typename F>
        void Resolve(F value) {
            _target->Dispatch(CreateTask([deferred = _deferred, value]() {
                node_webrtc::Resolve(deferred, value);
            }));
        }

        template <typename F>
        void Reject(F value) {
            _target->Dispatch(CreateTask([deferred = _deferred, value]() {
                node_webrtc::Reject(deferred, value);
            }));
        }

    private:

        napi_ref_ptr<T> _target;
        Napi::Promise::Deferred _deferred;
    };
} // namespace node_webrtc
