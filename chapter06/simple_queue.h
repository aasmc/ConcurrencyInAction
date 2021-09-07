#pragma once

#include "memory"
#include "utility"
#include "mutex"

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

    node *get_tail() {
        std::lock_guard tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::lock_guard head_lock(head_mutex);
        if (head.get == get_tail()) {
            return nullptr;
        }
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    simple_thread_safe_queue() : head(new node), tail(head.get()) {}

    simple_thread_safe_queue(const simple_thread_safe_queue &) = delete;

    simple_thread_safe_queue &operator=(const simple_thread_safe_queue &) = delete;

    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value))
        );
        std::unique_ptr<node> p(new node);
        node *const new_tail = p.get();
        std::lock_guard tail_lock(tail_mutex);
        tail->data = new_data;
        tail->next = std::move(p);
        tail = new_tail;
    }

};


































