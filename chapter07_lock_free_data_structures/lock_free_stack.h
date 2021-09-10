#pragma once

#include "atomic"
#include "memory"

template<typename T>
class lock_free_stack {
private:
    struct node {
        std::shared_ptr<T> data;
        node *next;

        node(const T &data_) : data(std::make_shared<T>(data_)) {}
    };

    std::atomic<node *> head;

public:
    void push(const T &data) {
        // step 1: create a new node, allocating memory in the heap
        // in case of exception our data structure is not touched
        node *const new_node = new node(data);
        // step 2: prepare the node before executing any atomic operation
        // set the new_node's next pointer to the head
        new_node->next = head.load();
        // step 3: atomically set the head pointer to the new_node
        // if it returns false to indicate that the comparison
        // failed (for example, because head was modified by another thread), the value
        // supplied as the first parameter ( new_node->next ) is updated to the current
        // value of head.
        while (!head.compare_exchange_weak(new_node->next, new_node));
    }

    std::shared_ptr<T> pop() {
        node *old_head = head.load();
        // here we first check that old_head is not nullPtr (it may be nullPtr
        // if the stack is empty, e.g.), and only then do we perform compare_exchange\
        // basically here we:
        // 1. Check that the stack is not empty
        // 2. Compare that head is the same as old_head
        // 3. If so, write old_head's next node to head
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
};

































