#pragma once

#include "memory"
#include "utility"

template<typename T>
class simple_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };

    std::unique_ptr<node> head;
    node *tail;

public:
    simple_queue() : head(new node), tail(head.get()) {}

    simple_queue(const simple_queue &) = delete;

    simple_queue &operator=(const simple_queue &) = delete;

    std::shared_ptr<T> try_pop() {
        if (head.get() == tail) {
            return std::shared_ptr<T>();
        }
        const std::shared_ptr<T> res(
                head->data
        );
        const std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return res;
    }

    void push(T new_value) {
        std::shared_ptr<T> new_data(
                std::make_shared<T>(std::move(new_value))
        );
        std::unique_ptr<node> p(new node);
        tail->data = new_data;
        node *const new_tail = p.get();
        tail->next = std::move(p);
        tail = new_tail;
    }

};


































