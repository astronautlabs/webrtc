#pragma once

#include "src/node/proxy.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory_reference.h"
#include <src/api/peer_connection_interface.h>
#include <src/api/scoped_refptr.h>

#define Super Proxy<ProxyT, NativeT>

namespace node_webrtc {
    template <typename ProxyT, typename NativeT>
    class FactoryOwnedProxy: public Proxy<ProxyT, NativeT> {
    public:
        FactoryOwnedProxy(const Napi::CallbackInfo& info):
            Super(info)
        {
        }

        napi_ref_ptr<PeerConnectionFactory> factory() { return _factory; }
    protected:
        void ConstructFromHandle(webrtc::scoped_refptr<NativeT> handle, Bundle args) override {
            this->_handle = handle;
            assert(this->_handle);

            if (args.HasFragment<PeerConnectionFactoryReference>())
                _factory = args.Fragment<PeerConnectionFactoryReference>().factory();
            else
                _factory = PeerConnectionFactory::GetOrCreateDefault();
            assert(_factory);
        }

        void Finalize(Napi::Env env) override {
            Super::Finalize(env);
            _factory = nullptr;
        }

        napi_ref_ptr<PeerConnectionFactory> _factory;
    };
} // namespace node_webrtc

#undef Super