/**
 * Copyright (c) 2021. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

/**
 * @brief A thread-safe, blocking queue generic data structure
 *
 * @tparam T queue element type
 */
template<typename T>
class BlockingQueue {
public:
    void push(T const &_data) {
        {
            std::lock_guard<std::mutex> lock(guard);
            queue.push(_data);
        }
        signal.notify_one();
    }

    bool tryPop(T &_value) {
        std::lock_guard<std::mutex> lock(guard);
        if (queue.empty()) {
            return false;
        }

        _value = queue.front();
        queue.pop();
        return true;
    }

    void waitAndPop(T &_value) {
        std::unique_lock<std::mutex> lock(guard);
        while (queue.empty()) {
            signal.wait(lock);
        }

        _value = queue.front();
        queue.pop();
    }


private:
    std::queue<T> queue;
    mutable std::mutex guard;
    std::condition_variable signal;
};
