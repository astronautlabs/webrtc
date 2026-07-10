
#include "src/converters.h"
#include "src/converters/napi.h" // IWYU: keep
#include <node-addon-api/napi.h>

namespace node_webrtc {

    class EventBuilder {
    public:
        explicit EventBuilder(Napi::Env env):
            _env(env),
            _object(Napi::Object::New(_env))
        {
        }

        EventBuilder& DispatchTo(std::function<void(Napi::Object)> dispatcher) {
            this->_dispatcher = dispatcher;
            return *this;
        }

        EventBuilder& With(const char* key, Napi::Value value) {
            _object.Set(key, value);
            return *this;
        }

        template <typename T>
        EventBuilder& With(const char* key, T value) {
            auto converted = From<Napi::Value>(std::make_pair(_env, value));
            assert(converted.IsValid());
            _object.Set(key, converted.FromValidation(_env.Undefined()));
            return *this;
        }

        EventBuilder& With(const char* key, const char* value) {
            return With(key, Napi::String::New(_env, value));
        }

        Napi::Object Build() {
            return _object;
        }

        void Dispatch() {
            if (_dispatcher)
                _dispatcher(Build());
        }
    private:
        std::function<void(Napi::Object)> _dispatcher;
        Napi::Env _env;
        Napi::Object _object;
    };
} // namespace node_webrtc