#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include <node-addon-api/napi.h>
#include <uv.h>

#include "absl/functional/any_invocable.h"
#include "src/node/event_builder.h"
#include "src/node/async_context_releaser.h"
#include "src/utilities/task.h"
#include "src/utilities/thread_safe_queue.h"
#include "src/utilities/log.h"

namespace node_webrtc {

    class ObjectTraces {
    public:
        static std::map<void*, absl::AnyInvocable<void()>>& subscribers() {
            static std::map<void*, absl::AnyInvocable<void()>> subs;
            return subs;
        }

        static void Notify() {
            for (auto& [ handle, action] : subscribers())
                action();
        }
    };

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
            ObjectTraces::subscribers()[this] = [&] { Inspect(); };
            AsyncContextReleaser::GetDefault();
            
            this->Ref();
        }

        virtual ~AsyncObjectWrapWithLoop() {
            ObjectTraces::subscribers().erase(this);
            DestroyAsyncContext();
        }
    private:
    
        ThreadSafeQueue<std::unique_ptr<Task>> queue;
        std::string _name = "<unnamed>";
        std::mutex _async_context_mutex;
        std::atomic<bool> _hasUV = false;
        std::atomic<bool> _uvKeepAlive = false;
        uv_async_t _async;
        Napi::AsyncContext* _context = nullptr;
        std::mutex _lock;
        std::atomic<bool> _shouldStop = false;
        std::atomic<bool> _didStop = false;

    protected:
        void DestroyAsyncContext() {
            Log(this, "Destroying async context");
            _async_context_mutex.lock();
            if (_context) {
                AsyncContextReleaser::SafeRelease(_context);
                _context = nullptr;
            }
            _async_context_mutex.unlock();
        }

    public:
        void Inspect() {
            Log(this, "=========================================================");
            Log(this, std::string { "UV:          " } + (_hasUV ? "Yes" : "No"));
            Log(this, std::string { "Async:       " } + (_context ? "Yes" : "No"));
            Log(this, std::string { "Should stop: " } + (_shouldStop ? "Yes" : "No"));
            Log(this, std::string { "Did stop:    " } + (_shouldStop ? "Yes" : "No"));
        }

        void Dispatch(std::unique_ptr<Task> event) {
            assert(_hasUV);
            queue.Enqueue(std::move(event));
            _lock.lock();
            if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(&_async))) {
                uv_async_send(&_async);
            }
            _lock.unlock();
        }

        bool should_stop() const { return _shouldStop; }

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
        void InitializeUV() {
            if (_hasUV)
                return;
            _hasUV = true;
            uv_loop_t* loop;
            auto status = napi_get_uv_event_loop(this->Env(), &loop);
            {
                using Napi::Error;
                NAPI_THROW_IF_FAILED_VOID(this->Env(), status);
            }

            Log(this, "uv: Creating async handle");
            uv_async_init(loop, &_async, [](auto handle) {
                auto self = static_cast<AsyncObjectWrapWithLoop<T>*>(handle->data);
                self->Run();
            });

            // do not hold the event loop open by default
            uv_unref(reinterpret_cast<uv_handle_t*>(&_async));
            _uvKeepAlive = false;
            _async.data = this;
        }

        /**
         * Adds a Napi::AsyncContext to this proxy. This should only be done for objects which make sense to 
         * expose via async_hooks, and definitely not for short-lived or "bursty" objects that would cause excessive
         * async_hooks notifications.
         */
        void InitializeAsyncContext() { 
            if (!_context) {
                Log(this, "Initializing async context");
                _context = new Napi::AsyncContext(this->Env(), _name.c_str(), this->Value());
            }
        }

        /**
         * Determines whether the Node.js event loop should be kept alive by this object. This should be the case for 
         * RTCPeerConnection, audio/video sinks, etc.
         */
        void SetKeepAlive(const bool &enabled) {
            assert(_hasUV);
            if (enabled != _uvKeepAlive) {
                _uvKeepAlive = enabled;
                if (enabled) {
                    uv_ref(reinterpret_cast<uv_handle_t*>(&_async));
                } else {
                    uv_unref(reinterpret_cast<uv_handle_t*>(&_async));
                }
            }
        }

        /**
         * This method will be invoked once the AsyncObjectWrapWithLoop stops.
         */
        virtual void DidStop() {
            Log(this, "DidStop()");
            _didStop = true;
            DestroyAsyncContext();
            // TODO(liam): This sort of thing is a half assed hack on bad reference handling. 
            // All object pointers should be held by napi_ref_ptr, allowing reference counts to be 
            // managed by RAII.
            // if (!this->IsEmpty())
            //     this->Unref();
        }

        virtual void Run() {
            Napi::HandleScope scope(this->Env());
            if (!_shouldStop) {
                while (auto event = queue.Dequeue()) {
                    // Not all objects have async contexts. If we do, set up the appropriate callback scope.
                    // If not, just run the task without it.
                    if (_context) {
                        Napi::CallbackScope callbackScope(this->Env(), *_context);
                        event->Execute();
                    } else {
                        event->Execute();
                    }

                    if (_shouldStop) {
                        break;
                    }
                }
            }
            if (_shouldStop) {
                Log(this, "AsyncObjectWrapWithLoop::Run(): Stopping due to signal");
                Log(this, "uv: Unregistering async handle");
                _lock.lock();
                uv_close(reinterpret_cast<uv_handle_t*>(&_async), [](auto handle) {
                    auto self = static_cast<AsyncObjectWrapWithLoop<T>*>(handle->data);
                    self->DidStop();
                });
                _hasUV = false;
                _lock.unlock();
            }
        }

        virtual void Stop() {
            Log(this, "AsyncObjectWrapWithLoop::Stop()");

            if (!_hasUV) {
                Log(this, "Stop() called without an active UV async handle");
                return;
            }
            
            Log(this, "AsyncObjectWrapWithLoop::Stop(): Signaling stop");
            _shouldStop = true;
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
