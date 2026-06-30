#pragma once

#include <mutex>
#include <queue>

#include <node-addon-api/napi.h>

#include "src/node/deferrer.h"

namespace node_webrtc {
    class AsyncContextReleaser : public Napi::ObjectWrap<AsyncContextReleaser> {
    public:
        AsyncContextReleaser(const Napi::CallbackInfo& info) :
            Napi::ObjectWrap<AsyncContextReleaser>(info),
            _deferrer(new ReleaserDeferrer(info.Env(), this)) {
        }

        ~AsyncContextReleaser() override;

        static AsyncContextReleaser* GetDefault();
        static void Init(Napi::Env, Napi::Object);

        static void SafeRelease(Napi::AsyncContext* context);

        void Release(Napi::AsyncContext*);

    protected:
        void Execute(Napi::Env);

    private:
        class ReleaserDeferrer : public Deferrer {
        public:
            ReleaserDeferrer(Napi::Env env, AsyncContextReleaser* parent) :
                Deferrer(env),
                _parent(parent) 
            {
            }

            void Execute(Napi::Env env) override {
                if (_parent) {
                    _parent->Execute(env);
                }
            }

            void Detach() { _parent = nullptr; }

        private:
            AsyncContextReleaser* _parent;
        };

        static AsyncContextReleaser* _default;
        static bool _teardown;
        static Napi::FunctionReference& constructor();

        ReleaserDeferrer* _deferrer;
        std::queue<Napi::AsyncContext*> _contexts;
        std::mutex _contexts_mutex;
    };

} // namespace node_webrtc
