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
    /**
     * Counts the number of threads trying to pop an item off the stack.
     * It is incremented at the beginning of pop() and decremented in
     * try_reclaim(), which is called once the node has been removed.
     */
    std::atomic<unsigned> threads_in_pop;

    std::atomic<node *> to_be_deleted;

    static void delete_nodes(node *nodes) {
        while (nodes) {
            node *next = nodes->next;
            delete nodes;
            nodes = next;
        }
    }

    void try_reclaim(node *old_head) {
        // this means that there's only one thread on pop() that is trying to delete a
        // node from the stack. So it is safe to delete a node that has just been removed.
        if (threads_in_pop == 1) {
            // claim the list of pending nodes
            node *nodes_to_delete = to_be_deleted.exchange(nullptr);
            // means no other thread can be accessing this list of pending nodes
            if (!--threads_in_pop) {
                // delete all nodes by iterating down the list
                delete_nodes(nodes_to_delete);
            } else if (nodes_to_delete) {
                // not safe to reclaim the nodes, so if there are any, we need to chain them
                // back onto the list of nodes pending deletion. This can happen if multiple threads are
                // accessing the data structure concurrently.
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        } else {
            // not safe to remove the node, so need to add it to the list of nodes pending deletion.
            chain_pending_nodes(old_head);
            --threads_in_pop;
        }
    }

    void chain_pending_nodes(node *nodes) {
        node *last = nodes;
        while (node *const next = last->next) {
            last = next;
        }
        chain_pending_nodes(nodes, last);
    }

    void chain_pending_nodes(node *first, node *last) {
        last->next = to_be_deleted;
        while (!to_be_deleted.compare_exchange_weak(last->next, first));
    }

    void chain_pending_node(node *n) {
        chain_pending_nodes(n, n);
    }

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
        ++threads_in_pop; // increase counter before doing anything else
        node *old_head = head.load();
        // here we first check that old_head is not nullPtr (it may be nullPtr
        // if the stack is empty, e.g.), and only then do we perform compare_exchange\
        // basically here we:
        // 1. Check that the stack is not empty
        // 2. Compare that head is the same as old_head
        // 3. If so, write old_head's next node to head
        while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
        std::shared_ptr<T> res;
        if (old_head) {
            res.swap(old_head->data); // reclaim deleted nodes if you can
        }
        try_reclaim(old_head);
        return res;
    }
};

































