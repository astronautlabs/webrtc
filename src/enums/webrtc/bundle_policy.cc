#include "src/enums/webrtc/bundle_policy.h"
#include <webrtc/api/peer_connection_interface.h>

namespace node_webrtc {
    static const bidirectional_map<webrtc::PeerConnectionInterface::BundlePolicy, std::string> BundlePolicyMap {
        {
            { webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyBalanced, "balanced" },
            { webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxCompat, "max-compat" },
            { webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle, "max-bundle" },
        }
    };

    ENUM_CLASS_CONVERTER_IMPL(webrtc::PeerConnectionInterface::BundlePolicy, BundlePolicyMap)
} // namespace node_webrtc