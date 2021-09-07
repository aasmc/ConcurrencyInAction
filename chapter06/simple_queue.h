#pragma once

#include "memory"
#include "utility"

template<typename T>
class simple_queue {
private:
    struct node {
        T data;
        std::unique_ptr<node> next;

        node(T data_) : data(std::move(data_)) {}

    };

    std::unique_ptr<node> head;
    node *tail;

public:
    simple_queue() : tail(nullptr) {}

    simple_queue(const simple_queue &) = delete;

    simple_queue &operator=(const simple_queue &) = delete;

    std::shared_ptr<T> try_pop() {
        if (!head) {
            return std::shared_ptr<T>();
        }
        const std::shared_ptr<T> res(
                std::make_shared<T>(std::move(head->data))
        );
        const std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        if (!head) {
            tail = nullptr;
        }
        return res;
    }

    void push(T new_value) {
        std::unique_ptr<node> p(new node(std::move(new_value)));
        node *const new_tail = p.get();
        if (tail) {
            tail->next = std::move(p);
        } else { // this means it is the first element in the queue
            head = std::move(p);
        }
        tail = new_tail;
    }

};


































