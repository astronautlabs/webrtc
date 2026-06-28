/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/utilities/bidi_map.h"
#include "src/utilities/napi_ref_ptr.h"
#include <src/api/scoped_refptr.h>

namespace node_webrtc {

    /**
     * Maintains a map between native webrtc objects and their node-webrtc proxies. 
     */
    template <typename NativeT, typename ProxyT>
    class ProxyRegistry {
    public:
        explicit ProxyRegistry(std::function<napi_ref_ptr<ProxyT>(webrtc::scoped_refptr<NativeT>, napi_ref_ptr<PeerConnectionFactory>)> createProxy) :
            _createProxy(createProxy) {
        }

        ProxyRegistry() = delete;
        ProxyRegistry(ProxyRegistry const&) = delete;
        ProxyRegistry& operator=(ProxyRegistry const&) = delete;

        napi_ref_ptr<ProxyT> Proxy(webrtc::scoped_refptr<NativeT> key, napi_ref_ptr<PeerConnectionFactory> factory = PeerConnectionFactory::GetOrCreateDefault()) {
            return _map.computeIfAbsent(key, [this, key, factory]() {
                return _createProxy(key, factory);
            });
        }

        webrtc::scoped_refptr<NativeT> Native(napi_ref_ptr<ProxyT> key) {
            return _map.get(key);
        }

        void Release(napi_ref_ptr<ProxyT> value) {
            _map.reverseRemove(value);
        }

    private:
        std::function<napi_ref_ptr<ProxyT>(webrtc::scoped_refptr<NativeT>, napi_ref_ptr<PeerConnectionFactory>)> _createProxy;
        BidiMap<webrtc::scoped_refptr<NativeT>, napi_ref_ptr<ProxyT>> _map;
    };

} // namespace node_webrtc
