#pragma once

#include "memory"
#include "utility"
#include "mutex"
#include "condition_variable"

/**
 * Unbounded thread safe queue. Internally uses custom singly linked list
 * with two pointers: head and tail. Head pointer is an std::shared_ptr<node>,
 * tail pointer is a raw pointer to node class. Struct node has two values:
 * std::shared_ptr<T> data that stores a pointer to data object of type T,
 * and std::unique_ptr<node> next that stores a reference to the next node in the list.
 *
 * Valid invariants:
 *  - tail->next == nullPtr
 *  - tail->data == nullPtr
 *  - head == tail implies an empty list
 *  - A single element list has head->next == tail
 *  - For each node x in the list, where x != tail, x->data points to an instance of T
 *  and x -> next points to the next node in the list. x->next == tail implies x is the last node in the list.
 *  - Following the next nodes from head will eventually yield tail.
 *
 * @tparam T type of values stored in the queue.
 */
template<typename T>
class simple_thread_safe_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::mutex head_mutex;
    std::mutex tail_mutex;
    std::unique_ptr<node> head;
    node *tail;
    std::condition_variable data_cond;

    node *get_tail() {
        std::lock_guard tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        data_cond.wait(head_lock, [&] { return head.get() != get_tail(); });
        return std::move(head_lock);
    }

    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }

    std::unique_ptr<node> wait_pop_head(T &value) {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head(T &value) {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail()) {
            return std::unique_lock<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }

public:
    simple_thread_safe_queue() : head(new node), tail(head.get()) {}

    simple_thread_safe_queue(const simple_thread_safe_queue &) = delete;

    simple_thread_safe_queue &operator=(const simple_thread_safe_queue &) = delete;

    std::shared_ptr<T> wait_and_pop() {
        const std::unique_ptr<node> old_head = wait_pop_head();
        return old_head->data;
    }

    void wait_and_pop(T &value) {
        const std::unique_ptr<node> old_head = wait_pop_head(value);
    }

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool try_pop(T &new_value) {
        std::unique_ptr<node> old_head = try_pop_head(new_value);
        return old_head;
    }

    bool empty() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }

    void push(T new_value) {
        // allocating memory for new_data and p outside a lock
        // provides for greater concurrency
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value))
        );
        std::unique_ptr<node> p(new node);
        // only one an add its new node to the list at a time, but the code
        // to do so is only a few simple pointer assignments, so the lock isn’t held for much time
        {
            std::lock_guard tail_lock(tail_mutex);
            tail->data = new_data;
            node *const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        // at this point the mutex is unlocked and another thread can proceed at once
        data_cond.notify_one();
    }
};


































