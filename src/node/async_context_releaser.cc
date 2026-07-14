#include "src/node/async_context_releaser.h"
#include "src/utilities/log.h"
#include <node-addon-api/napi.h>

namespace node_webrtc {

    AsyncContextReleaser* AsyncContextReleaser::_default = nullptr;
    bool AsyncContextReleaser::_teardown = false;

    AsyncContextReleaser::AsyncContextReleaser(const Napi::CallbackInfo& info):
        Napi::ObjectWrap<AsyncContextReleaser>(info) 
    {

        // Initialize the ThreadSafeFunction
        _tsfn = Napi::ThreadSafeFunction::New(
            info.Env(),
            Napi::Function::New(info.Env(), [](const Napi::CallbackInfo&) { }), // Dummy JS Callback
            "AsyncContextReleaser",
            0, // Unlimited queue size
            1 // Initial thread count
        );
        // Don't keep the process running waiting for this TSFN to abort.
        _tsfn.Unref(info.Env());
    }

    AsyncContextReleaser::~AsyncContextReleaser() {
        if (_default == this) {
            _default = nullptr;
            _teardown = true;

            // Tell the OS we are done using this thread-safe function
            _tsfn.Release();
        }
    }

    Napi::FunctionReference& AsyncContextReleaser::constructor() {
        static Napi::FunctionReference constructor;
        return constructor;
    }

    void AsyncContextReleaser::Release(Napi::AsyncContext* context) {
        _contexts_mutex.lock();
        _contexts.push(context);
        _contexts_mutex.unlock();

        // Signal the TSFN to wake up the main event loop
        if (!_teardown) {
            auto callback = [](Napi::Env env, Napi::Function /*jsCb*/, AsyncContextReleaser* self) {
                if (self && !AsyncContextReleaser::_teardown) {
                    self->Execute(env);
                }
            };
            _tsfn.NonBlockingCall(this, callback);
        }
    }

    void AsyncContextReleaser::Execute(Napi::Env env) {
        _contexts_mutex.lock();
        while (!_contexts.empty()) {
            Napi::HandleScope scope(env);
            auto* context = _contexts.front();
            delete context;
            _contexts.pop();
        }
        _contexts_mutex.unlock();
    }

    AsyncContextReleaser* AsyncContextReleaser::GetDefault() {
        if (_teardown)
            return nullptr;
        if (!_default) {
            Napi::HandleScope scope(constructor().Env());
            auto object = constructor().New({});
            _default = Unwrap(object);
            _default->Ref();
        }
        return _default;
    }

    void AsyncContextReleaser::Init(Napi::Env env, Napi::Object) {
        auto func = DefineClass(env,
            "AsyncContextReleaser", 
            {

            });
        constructor() = Napi::Persistent(func);
        constructor().SuppressDestruct();

        // If we wait for ~AsyncContextReleaser, it may be too late for the _teardown flag to be effective
        // depending on finalization order. This hook ensures that we stop doing async context releases
        // as soon as the process begins cleaning up.
        env.AddCleanupHook([]() {
            Log<AsyncContextReleaser>("Detected Node.js cleanup started.");
            AsyncContextReleaser::_teardown = true;
            if (_default) {
                // Sever the TSFN so background threads can't queue more events and
                // flush the queue manually one last time before the process dies
                _default->_tsfn.Abort();
                _default->Execute(_default->Env());
            }
        });
    }

    void AsyncContextReleaser::SafeRelease(Napi::AsyncContext* context) {
        auto* instance = GetDefault();
        if (instance) {
            instance->Release(context);
        } else {
            // We are in process teardown. The event loop is dead anyway,
            // so just free the memory natively to prevent leaks.
            delete context;
        }
    }

} // namespace node_webrtc
