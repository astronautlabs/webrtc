#include <memory>
#include <api/scoped_refptr.h>

template <typename T>
inline webrtc::scoped_refptr<T> make_scoped_refptr(T* value) {
    return webrtc::scoped_refptr<T> { value };
}
