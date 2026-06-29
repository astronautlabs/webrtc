#pragma once

#include <atomic>
#include <mutex>

#include <node-addon-api/napi.h>
#include <uv.h>

#include "absl/functional/any_invocable.h"
#include "src/node/event_builder.h"
#include "src/node/async_context_releaser.h"
#include "src/utilities/task.h"
#include "src/utilities/thread_safe_queue.h"

namespace node_webrtc {
    template <typename T>
    class AsyncObjectWrapWithLoop: public Napi::ObjectWrap<T> {
    public:
        AsyncObjectWrapWithLoop(
            std::string name,
            T& target,
            const Napi::CallbackInfo& info
        ) :
            Napi::ObjectWrap<T>(info),
            _name(std::move(name))
        {
            AsyncContextReleaser::GetDefault();

            uv_loop_t* loop;
            auto status = napi_get_uv_event_loop(this->Env(), &loop);
            {
                using Napi::Error;
                NAPI_THROW_IF_FAILED_VOID(this->Env(), status);
            }

            uv_async_init(loop, &_async, [](auto handle) {
                auto self = static_cast<AsyncObjectWrapWithLoop<T>*>(handle->data);
                self->Run();
            });

            _async.data = this;
            
            this->Ref();
        }

        virtual ~AsyncObjectWrapWithLoop() {
            DestroyAsyncContext();
        }
    private:
    
        ThreadSafeQueue<std::unique_ptr<Task>> queue;
        std::string _name = "<unnamed>";
        std::mutex _async_context_mutex;
        uv_async_t _async;
        Napi::AsyncContext* _context = nullptr;
        std::mutex _lock;
        std::atomic<bool> _should_stop = { false };

        void DestroyAsyncContext() {
            _async_context_mutex.lock();
            if (_context) {
                AsyncContextReleaser::SafeRelease(_context);
                _context = nullptr;
            }
            _async_context_mutex.unlock();
        }

    public:

        void Dispatch(std::unique_ptr<Task> event) {
            queue.Enqueue(std::move(event));
            _lock.lock();
            if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&_async))) {
                uv_async_send(&_async);
            }
            _lock.unlock();
        }

        bool should_stop() const { return _should_stop; }

        void OnNodeThread(absl::AnyInvocable<void()> callback) {
            Dispatch(CreateTask(std::move(callback)));
        }

        EventBuilder Event(const char* type) {
            return EventBuilder { this->Env() }
                .With("type", type)
                .DispatchTo([&](Napi::Object event) {
                this->MakeCallback("dispatchEvent", { event });
            });
        }

    protected:
        /**
         * Adds a Napi::AsyncContext to this proxy. This should only be done for objects which make sense to 
         * expose via async_hooks, and definitely not for short-lived or "bursty" objects that would cause excessive
         * async_hooks notifications.
         */
        void InitializeAsyncContext() { 
            if (!_context)
                _context = new Napi::AsyncContext(this->Env(), _name.c_str(), this->Value());
        }

        /**
         * This method will be invoked once the AsyncObjectWrapWithLoop stops.
         */
        virtual void DidStop() {
            if (!this->IsEmpty())
                this->Unref();
        }

        virtual void Run() {
            Napi::HandleScope scope(this->Env());
            if (!_should_stop) {
                while (auto event = queue.Dequeue()) {
                    // Not all objects have async contexts. If we do, set up the appropriate callback scope.
                    // If not, just run the task without it.
                    if (_context) {
                        Napi::CallbackScope callbackScope(this->Env(), *_context);
                        event->Execute();
                    } else {
                        event->Execute();
                    }

                    if (_should_stop) {
                        break;
                    }
                }
            }
            if (_should_stop) {
                _lock.lock();
                uv_close(reinterpret_cast<uv_handle_t*>(&_async), [](auto handle) {
                    auto self = static_cast<AsyncObjectWrapWithLoop<T>*>(handle->data);
                    self->DidStop();
                });
                _lock.unlock();
            }
        }

        virtual void Stop() {
            _should_stop = true;
            Dispatch(Task::Create());
        }
        
        void MakeCallback(const char* name, const std::initializer_list<napi_value>& args) {
            auto self = this->Value();
            auto maybeFunction = self.Get(name);
            if (maybeFunction.IsFunction()) {
                _async_context_mutex.lock();
                maybeFunction.template As<Napi::Function>().MakeCallback(self, args, _context ? *_context : nullptr);
                _async_context_mutex.unlock();
            }
        }
    };

} // namespace node_webrtc
