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

#include <memory>
#include <mutex>
#include <queue>

namespace node_webrtc {

    /**
     * A thread safe queue which supports move-only objects. 
     */
    template <typename T>
    class ThreadSafeQueue {
    public:
        /**
         * Enqueue an Event.
         * @param event the event to enqueue
         */
        void Enqueue(T event) {
            _mutex.lock();
            _events.push(std::move(event));
            _mutex.unlock();
        }

        /**
         * Attempt to dequeue an Event. If the queue is empty, this method
         * returns nullptr.
         * @return the dequeued Event or nullptr
         */
        T Dequeue() {
            _mutex.lock();
            if (_events.empty()) {
                _mutex.unlock();
                return nullptr;
            }
            auto event = std::move(_events.front());
            _events.pop();
            _mutex.unlock();
            return event;
        }

    private:
        std::queue<T> _events;
        std::mutex _mutex { };
    };

} // namespace node_webrtc
