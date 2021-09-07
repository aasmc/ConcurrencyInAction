#pragma once

#include "queue"
#include "memory"
#include "mutex"
#include "condition_variable"

/**
 * There’s a slight twist with regard to exception safety in that if more than one
 * thread is waiting when an entry is pushed onto the queue, only one thread will be
 * woken by the call to data_cond.notify_one() . But if that thread then throws an
 * exception in wait_and_pop() , such as when the new std::shared_ptr<> is constructed,
 *  none of the other threads will be woken.
 *
 *  If this isn’t acceptable, the call is
 * readily replaced with data_cond.notify_all() , which will wake all the threads but at
 * the cost of most of them then going back to sleep when they find that the queue is
 * empty after all. A second alternative is to have wait_and_pop() call notify_one() if
 * an exception is thrown, so that another thread can attempt to retrieve the stored
 * value. A third alternative is to move the std::shared_ptr<> initialization to the
 * push() call and store std::shared_ptr<> instances rather than direct data values.
 * Copying the std::shared_ptr<> out of the internal std::queue<> then can’t throw
 * an exception, so wait_and_pop() is safe again.
 */
template<typename T>
class ThreadSafeQueueRevised {
private:
    mutable std::mutex mut;
    std::queue<std::shared_ptr<T>> dataQueue;
    /**
     * Use condition variable to tell other threads that the queue is not empty.
     */
    std::condition_variable dataCond;

public:
    ThreadSafeQueueRevised() = default;

    ThreadSafeQueueRevised &operator=(const ThreadSafeQueueRevised &) = delete;

    ThreadSafeQueueRevised(const ThreadSafeQueueRevised &other) {
        std::lock_guard lk(other.mut);
        dataQueue = other.dataQueue;
    }

    void Push(T newValue) {
        std::shared_ptr<T> data(
                std::make_shared<T>(std::move(newValue))
        );
        std::lock_guard lk(mut);
        dataQueue.push(data);
        // here we notify a waiting thread that this queue is not empty any longer
        dataCond.notify_one();
    }

    void WaitAndPop(T &value) {
        std::unique_lock lk(mut);
        dataCond.wait(lk, [this] { return !dataQueue.empty() });
        value = std::move(*dataQueue.front());
        dataQueue.pop();
    }

    std::shared_ptr<T> WaitAndPop() {
        // use unique lock because it allows to lock and unlock when necessary
        // lock_guard doesn't allow it.
        std::unique_lock lk(mut);
        // thread trying to pop an element from queue will sleep waiting for the
        // queue to be not empty any longer.
        dataCond.wait(lk, [this] { return !dataQueue.empty() });
        std::shared_ptr<T> res = dataQueue.front();
        dataQueue.pop();
        return res;
    }

    bool TryPop(T &value) {
        std::lock_guard lk(mut);
        if (dataQueue.empty()) {
            return false;
        }
        value = std::move(*dataQueue.front());
        dataQueue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop() {
        std::lock_guard lk(mut);
        if (dataQueue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res = dataQueue.front();
        dataQueue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard lk(mut);
        return dataQueue.empty();
    }
};





















