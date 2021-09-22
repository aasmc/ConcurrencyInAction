#pragma once

#include "atomic"
#include "memory"

template<typename T>
class lock_free_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        node *next;

        node() : next(nullptr) {}
    };

    std::atomic<node *> head;
    std::atomic<node *> tail;

    node *pop_head() {
        const node *old_head = head.load();
        if (old_head == tail.load()) { // 1
            return nullptr;
        }
        head.store(old_head->next);
        return old_head;
    }

public:
    lock_free_queue() : head(new node), tail(head.load()) {};

    ~lock_free_queue() {
        while (const node *old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    lock_free_queue(const lock_free_queue &) = delete;

    lock_free_queue &operator=(const lock_free_queue &) = delete;

    std::shared_ptr<T> pop() {
        node *old_head = pop_head();
        if (!old_head) {
            return std::shared_ptr<T>();
        }
        const std::shared_ptr<T> res(old_head->data); // 2
        delete old_head;
        return res;
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(new_value));
        node *p = new node;  // 3
        const node *old_tail = tail.load();  // 4
        old_tail->data.swap(new_data);  // 5
        old_tail->next = p;  // 6
        tail.store(p) // 7
    }
};































