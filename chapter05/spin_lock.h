#pragma once

#include "atomic"

class spinlock_mutex {
    std::atomic_flag flag;
public:
    // atomic flag MUST be initialized with this value
    spinlock_mutex() : flag(ATOMIC_FLAG_INIT) {}

    void lock() {
        // atomic read write operation
        // 1: get previous value
        // 2: check if it is true, then write true to it again and spin in the loop
        // 3: if it is false then write true to it and break out of the loop
        while (flag.test_and_set(std::memory_order_acquire));
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};