#pragma once

#include "atomic"
#include "memory"
#include "thread"
#include "stdexcept"
#include "functional"

namespace refcount {
    /**
     * Lock free stack implementation that uses split reference count technique to
     * count the number of threads trying to delete a node from the stack.
     *
     * One possible technique involves the use of not one but two reference counts for
     * each node: an internal count and an external count. The sum of these values is the
     * total number of references to the node. The external count is kept alongside the pointer
     * to the node and is increased every time the pointer is read. When the reader is fin-
     * ished with the node, it decreases the internal count. A simple operation that reads the
     * pointer will leave the external count increased by one and the internal count decreased
     * by one when it’s finished.
     *
     * When the external count/pointer pairing is no longer required (the node is no
     * longer accessible from a location accessible to multiple threads), the internal count is
     * increased by the value of the external count minus one and the external counter is
     * discarded. Once the internal count is equal to zero, there are no outstanding refer-
     * ences to the node and it can be safely deleted. It’s still important to use atomic opera-
     * tions for updates of shared data.
     *
     * @tparam T
     */
    template<typename T>
    class lock_free_stack {
        struct node;
        /**
         * External count is wrapped together with the node pointer.
         */
        struct counted_node_ptr {
            int external_count;
            node *ptr;
        };

        struct node {
            std::shared_ptr<T> data;
            std::atomic<int> internal_count;
            counted_node_ptr next;

            node(const T &data_) :
                    data(std::make_shared<T>(data_)),
                    internal_count(0) {}
        };

        std::atomic<counted_node_ptr> head;

    public:
        ~lock_free_stack() {
            while (pop());
        }

        void push(const T &data) {
            counted_node_ptr new_node;
            new_node.ptr = new node(data);
            new_node.external_count = 1;
            new_node.ptr->next = head.load(std::memory_order_relaxed);
            while (!head.compare_exchange_weak(new_node.ptr->next, new_node,
                                               std::memory_order_release,
                                               std::memory_order_relaxed));
        }


    private:
        void increase_head_count(counted_node_ptr &old_counter) {
            counted_node_ptr new_counter;
            do {
                new_counter = old_counter;
                ++new_counter.external_count
            } while (!head.compare_exchange_strong(old_counter, new_counter,
                                                   std::memory_order_acquire,
                                                   std::memory_order_relaxed)); // 1
            old_counter.external_count = new_counter.external_count
        }

    public:
        std::shared_ptr<T> pop() {
            counted_node_ptr old_head = head.load(std::memory_order_relaxed);
            for (;;) {
                // first increase the count of external references to the head node to indicate
                // that we are referencing it and to ensure that it's safe to dereference it.
                // if we dereference the pointer BEFORE increasing the ref count, another thread could free
                // the node before you access it, leaving us with a dangling pointer.
                increase_head_count(old_head);
                node *const ptr = old_head.ptr; // 2
                if (!ptr) { // means we are at the end of the list: no more entries.
                    return std::shared_ptr<T>();
                }

                if (head.compare_exchange_strong(old_head, ptr->next,
                                                 std::memory_order_relaxed)) { // 3 try to remove the node
                    // if succeeds, we have taken the ownership of the node, and cap swap out the data
                    // in preparation for returning it.
                    std::shared_ptr<T> res;
                    res.swap(ptr->data); // 4
                    // we removed the node from the list, that's -1,
                    // and we no longer access the node from this thread, that's -1 as well
                    // in total it is -2
                    const int count_increase = old_head.external_count - 2; // 5
                    // add the external count to the internal count
                    // after fetch_add we get the previous value stored in internal_count
                    // if previous value in internal_count ==  -count_increase, this means,
                    // that after adding the two values we get 0, so we can safely remove the pointer
                    if (ptr->internal_count.fetch_add(count_increase,
                                                      std::memory_order_release) == -count_increase) { // 6
                        delete ptr;
                    }
                    // whether we remove the node or not, we have finished processing the list,
                    // and we can return the data.
                    return res; // 7
                    // if compare_exchange fails, another thread removed our node before we
                    // did, or another thread added a new node to the stack. Either way, we need to start
                    // again with the fresh value of head returned by the compare_exchange call. But first
                    // we must decrease the ref count on the node we were trying to remove. This thread won't
                    // access it anymore. If we are the last thread to hold a ref (because another thead removed it
                    // from the stack) the internal ref count will be 1, so subtracting 1 will set
                    // the count to zero. In this case, we can delete the node here before another loop iteration.
                } else if (ptr->internal_count.fetch_sub(1, std::memory_order_relaxed) == 1) {
                    ptr->internal_count.load(std::memory_order_acquire)
                    delete ptr; // 8
                }
            }
        }
    };
}



























