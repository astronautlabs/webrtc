#pragma once 

#include "src/utilities/nameof.h"
#include "src/utilities/napi_ref_ptr.h"
#include <concepts>
#include <map>
#include <node-addon-api/napi.h>

namespace node_webrtc {
    class Addon: public Napi::Addon<Addon>, public NapiRefTeardownIndicator {
    public:
        Addon(Napi::Env env, Napi::Object exports);
        
        static Addon& instance(Napi::Env env) {
            auto* addon = env.GetInstanceData<Addon>();
            assert(addon);
            return *addon;
        }

        template <typename T>
        static T& Fragment(Napi::Env env) {
            return instance(env).GetFragment<T>();
        }

        template <typename T>
        T& GetFragment() {
            const auto fragmentName = std::string { NAMEOF_TYPE(T) };
            if (!fragments.contains(fragmentName)) {
                T* fragment = nullptr;

                if constexpr (std::constructible_from<T, Napi::Env>) {
                    fragment = new T(_env);
                } else {
                    fragment = new T();
                }

                fragments[fragmentName] = fragment;
            }

            return *static_cast<T*>(fragments[fragmentName]);
        }
    private:
        Napi::Env _env;
        std::map<std::string, void*> fragments;
    };
}
