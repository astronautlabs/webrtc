/**
 * Copyright (c) 2022 Astronaut Labs, LLC. All rights reserved.
 * Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <functional>
#include <memory>
#include <absl/functional/any_invocable.h>

namespace node_webrtc {
    /**
     * A container for a simple task invocable, intended to be run on another thread.
     * TODO(liam): Uses absl::AnyInvocable because std::move_only_function isn't available yet.
     */
    class Task {
    public:
        void Execute() {
            _action();
        }

        virtual ~Task() = default;

        static std::unique_ptr<Task> Create(absl::AnyInvocable<void()> callback) {
            return std::unique_ptr<Task>(new Task(std::move(callback)));
        }

        static std::unique_ptr<Task> Create() {
            return std::unique_ptr<Task>(new Task([] {}));
        }

    private:
        explicit Task(absl::AnyInvocable<void()> action) :
            _action(std::move(action)) { }
        absl::AnyInvocable<void()> _action;
    };

    std::unique_ptr<Task> CreateTask(absl::AnyInvocable<void()> callback);

} // namespace node_webrtc
