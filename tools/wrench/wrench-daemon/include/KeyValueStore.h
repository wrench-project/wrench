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
 * @brief A thread-safe key-value store data structure, where keys are strings
 * @tparam T value type
 */
template<typename T>
class KeyValueStore {
public:
    void insert(const std::string &_key, T const &_value) {
        std::lock_guard <std::mutex> lock(guard);
        store[_key] = _value;
    }

    bool lookup(const std::string &_key, T &_value) {
        std::lock_guard <std::mutex> lock(guard);
        if (store.find(_key) == store.end()) {
            return false;
        }
        _value = store[_key];
        return true;
    }

    bool remove(const std::string &_key) {
        std::lock_guard <std::mutex> lock(guard);
        if (store.find(_key) == store.end()) {
            return false;
        }
        store.erase(_key);
        return true;
    }

private:
    std::map <std::string, T> store;
    mutable std::mutex guard;
};
