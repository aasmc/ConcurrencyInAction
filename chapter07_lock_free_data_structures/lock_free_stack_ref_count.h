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
     * tions for updates of shared data. Let’s now look at an implementation of a lock-free
     * stack that uses this technique to ensure that the nodes are reclaimed only when it’s
     * safe to do so.
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
            new_node.ptr->next = head.load();
            while (!head.compare_exchange_weak(new_node.ptr->next, new_node));
        }
    };
}



























