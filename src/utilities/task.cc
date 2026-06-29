#include "task.h"

namespace node_webrtc {
    std::unique_ptr<Task> CreateTask(absl::AnyInvocable<void()> callback) {
        return Task::Create(std::move(callback));
    }
} // namespace node_webrtc