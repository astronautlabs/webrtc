#pragma once 

#include "src/utilities/nameof.h"
#include <stdexcept>
#include <string>
#include <map>

namespace node_webrtc {
    class Bundle {
        std::map<std::string, void*> fragments;
    public:
        template <typename T>
        T& RequireFragment() {
            const auto fragmentName = std::string { NAMEOF_TYPE(T) };
            if (!fragments.contains(fragmentName))
                throw std::invalid_argument { std::string { "Must contain fragment " } + NAMEOF_TYPE(T) };

            return Fragment<T>();
        }

        template <typename T>
        T& Fragment() {
            const auto fragmentName = std::string { NAMEOF_TYPE(T) };
            if (!fragments.contains(fragmentName)) {
                T* fragment = nullptr;
                fragment = new T();
                fragments[fragmentName] = fragment;
            }

            return *static_cast<T*>(fragments[fragmentName]);
        }

        template <typename T, typename Self>
        Self&& AddFragment(this Self&& self, const T&& fragmentTemplate) {
            const auto fragmentName = std::string { NAMEOF_TYPE(T) };
            if (self.fragments.contains(fragmentName))
                throw std::invalid_argument { std::string { "Already contains fragment " } + std::string { NAMEOF_TYPE(T) } };

            T* fragment = nullptr;
            fragment = new T();
            *fragment = fragmentTemplate;
            self.fragments[fragmentName] = fragment;
            return self;
        }
    };
} // namespace node_webrtc
