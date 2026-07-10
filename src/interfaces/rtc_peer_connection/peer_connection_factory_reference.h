#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/utilities/napi_ref_ptr.h"
namespace node_webrtc {
    class PeerConnectionFactoryReference {
        napi_ref_ptr<PeerConnectionFactory> _factory;
    public:
        explicit PeerConnectionFactoryReference(napi_ref_ptr<PeerConnectionFactory> factory): 
            _factory(factory)
        {
        }

        auto factory() { return _factory; }
    };
} // namespace node_webrtc