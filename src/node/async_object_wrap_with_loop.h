#pragma once

#include <atomic>
#include <mutex>

#include <node-addon-api/napi.h>

#include "src/node/async_object_wrap.h"
#include "src/node/event_loop.h"
#include "src/node/event_builder.h"

namespace node_webrtc {
    
    template <typename T>
    class AsyncObjectWrapWithLoop
        : public AsyncObjectWrap<T>,
          public EventLoop<T> {
    public:
        AsyncObjectWrapWithLoop(
            std::string name,
            T& target,
            const Napi::CallbackInfo& info
        ) :
            AsyncObjectWrap<T>(name, info),
            EventLoop<T>(name, info.Env(), this->context(), target) {
            this->Ref();
        }

        void OnNodeThread(std::function<void()> callback) {
            EventLoop<T>::Dispatch(CreateCallback<T>([callback]() {
                callback();
            }));
        }

        EventBuilder Event(const char* type) {
            return EventBuilder { this->Env() }
                .With("type", type)
                .DispatchTo([&] (Napi::Object event) {
                    this->MakeCallback("dispatchEvent", { event });
                })
            ;
        }

    protected:
        /**
         * This method will be invoked once the AsyncObjectWrapWithLoop stops.
         */
        void DidStop() override {
            if (!this->IsEmpty())
                this->Unref();
        }
    };

} // namespace node_webrtc
