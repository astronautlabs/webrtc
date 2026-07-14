#pragma once

#include <mutex>
#include <queue>

#include <node-addon-api/napi.h>

namespace node_webrtc {
    class AsyncContextReleaser : public Napi::ObjectWrap<AsyncContextReleaser> {
    public:
        AsyncContextReleaser(const Napi::CallbackInfo& info);
        ~AsyncContextReleaser() override;

        static AsyncContextReleaser* GetDefault();
        static void Init(Napi::Env, Napi::Object);

        static void SafeRelease(Napi::AsyncContext* context);

        void Release(Napi::AsyncContext*);

    protected:
        void Execute(Napi::Env);

    private:
        static AsyncContextReleaser* _default;
        static bool _teardown;
        static Napi::FunctionReference& constructor();

        Napi::ThreadSafeFunction _tsfn;
        std::queue<Napi::AsyncContext*> _contexts;
        std::mutex _contexts_mutex;
    };

} // namespace node_webrtc