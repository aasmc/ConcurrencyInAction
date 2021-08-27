#pragma once

#include "exception"
#include "memory"
#include "mutex"
#include "stack"

struct EmptyStack : std::exception {
    const char *what() const throw();
};

template<typename T>
class ThreadSafeStack {
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    ThreadSafeStack() {}

    ThreadSafeStack(const ThreadSafeStack &other) {
        std::lock_guard<std::mutex> lock(other.m);
        data = other.data;
    }

    ThreadSafeStack &operator=(const ThreadSafeStack &) = delete;

    void Push(T newValue) {
        std::lock_guard lock(m);
        data.push(std::move(newValue));
    }

    /**
     * Returns a pointer to the element at the back of the stack and removes
     * the element from the stack.
     * @return
     */
    std::shared_ptr<T> Pop() {
        std::lock_guard lock(m);
        // check for empty before trying to pop value
        if (data.empty()) throw EmptyStack();
        // allocate return value before modifying the data stack
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }

    /**
     * Removes the element from the back of the stack and
     * makes [value] refer to the removed element.
     * @param value
     */
    void Pop(T &value) {
        std::lock_guard lock(m);
        if (data.empty()) throw EmptyStack();
        value = data.top();
        data.pop();
    }

    bool Empty() const {
        std::lock_guard lock(m);
        return data.empty();
    }

};
































