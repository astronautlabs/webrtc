#include "src/utilities/napi_ref_ptr.h"
#include <concepts>
#include <webrtc/api/scoped_refptr.h>

/**
 * Unified Cast<T>(U u) utilities for smart pointers.
 */

/**
 * Cast from one webrtc class to another statically, wrapping it back into a webrtc::scoped_refptr
 */
template <typename ChildT, typename BaseT> 
    requires std::derived_from<ChildT, BaseT>
webrtc::scoped_refptr<ChildT> Cast(webrtc::scoped_refptr<BaseT> base) {
    return webrtc::scoped_refptr { static_cast<ChildT*>(base.get()) };
}

/**
 * Cast from one Node.js class to another statically, wrapping it back into a napi_ref_ptr
 */
template <typename ChildT, typename BaseT> 
    requires std::derived_from<ChildT, BaseT>
napi_ref_ptr<ChildT> Cast(napi_ref_ptr<BaseT> base) {
    return napi_ref_ptr { static_cast<ChildT*>(base.get()) };
}