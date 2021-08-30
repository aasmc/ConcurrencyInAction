#pragma once

#include "queue"
#include "memory"
#include "mutex"
#include "condition_variable"

template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mut;
    std::queue<T> dataQueue;
    /**
     * Use condition variable to tell other threads that the queue is not empty.
     */
    std::condition_variable dataCond;

public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue & operator=(const ThreadSafeQueue&) = delete;

    ThreadSafeQueue(const ThreadSafeQueue &other) {
        std::lock_guard lk(other.mut);
        dataQueue = other.dataQueue;
    }

    void Push(T newValue) {
        std::lock_guard lk(mut);
        dataQueue.push(newValue);
        // here we notify a waiting thread that this queue is not empty any longer
        dataCond.notify_one();
    }

    void WaitAndPop(T &value) {
        std::unique_lock lk(mut);
        dataCond.wait(lk, [this] { return !dataQueue.empty() });
        value = dataQueue.front();
        dataQueue.pop();
    }

    std::shared_ptr<T> WaitAndPop() {
        // use unique lock because it allows to lock and unlock when necessary
        // lock_guard doesn't allow it.
        std::unique_lock lk(mut);
        // thread trying to pop an element from queue will sleep waiting for the
        // queue to be not empty any longer.
        dataCond.wait(lk, [this] { return !dataQueue.empty() });
        std::shared_ptr<T> res(std::make_shared<T>(dataQueue.front()));
        dataQueue.pop();
        return res;
    }

    bool TryPop(T &value) {
        std::lock_guard lk(mut);
        if (dataQueue.empty()) {
            return false;
        }
        value = dataQueue.front();
        dataQueue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop() {
        std::lock_guard lk(mut);
        if (dataQueue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(dataQueue.front()));
        dataQueue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard lk(mut);
        return dataQueue.empty();
    }
};





















