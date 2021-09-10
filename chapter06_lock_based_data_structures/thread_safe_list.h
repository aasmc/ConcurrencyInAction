#pragma once

#include "vector"
#include "utility"
#include "functional"
#include "list"
#include "mutex"
#include "shared_mutex"
#include "algorithm"
#include "numeric"
#include "map"

/**
* A simple thread safe list with iterator support.
 * It supports the following operations:
 *
 * - Add an item to the list.
 * - Remove an item from the list if it meets a certain condition.
 * - Find an item in the list that meets a certain condition.
 * - Update an item that meets a certain condition.
 * - Copy each item in the list to another container.
 *
 * The list is intended to be used as part of the thread safe lookup table,
 * so some of the usual operations on lists are not supported.
 * E.g. it has no positional insert.
 *
 * The basic idea with fine-grained locking for a linked list is to have one
 * mutex per node. If the list gets big, that’s a lot of mutexes!
 * The benefit here is that operations on separate parts of the list are truly concurrent:
 * each operation holds only the locks on the nodes it’s interested in
 * and unlocks each node as it moves on to the next.
 *
 * This is a singly linked list where each entry is a node structure.
 *
 * Different threads can be working on different nodes
 * in the list at the same time, whether they’re processing each item with for_each() ,
 * searching with find_first_if() , or removing items with remove_if() . But because
 * the mutex for each node must be locked in turn, the threads can’t pass each other. If
 * one thread is spending a long time processing a particular node, other threads will
 * have to wait when they reach that particular node.
*/
template<typename T>
class thread_safe_list {
    struct node {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node() : next() {}

        node(const T &value) : data(std::make_shared<T>(value)) {}
    };

    /**
     * Default constructed node is used as a head of the list.
     */
    node head;

public:
    thread_safe_list() {}

    ~thread_safe_list() {
        remove_if([](const node &) { return true; })
    }

    thread_safe_list(const thread_safe_list &other) = delete;

    thread_safe_list &operator=(const thread_safe_list &other) = delete;

    void push_front(const T &value) {
        // 1: construct a new node by allocating stored data on the heap. next pointer of the node is nullPtr
        // slow memory allocation happens outside the lock
        std::unique_ptr<node> new_node(new node(value));
        // 2: lock the head node to provide mutual exclusion
        // since we lock only 1 mutex, there's no risk of deadlock
        std::lock_guard lk(head.m);
        // 3: now the new node's next points to the second element in the list
        new_node->next = std::move(head.next);
        // 4: and the head points to the new node
        head.next = std::move(new_node);
    }

    /**
     * Applies a given function [f] of type Function to every element in the list.
     * In common with most standard library algorithms it accepts the Function by
     * value.
     * @tparam Function type of the function that will be applied to elements in the list.
     *          It must have only one parameter of type T.
     * @param f
     */
    template<class Function>
    void for_each(Function f) {
        node *current = &head;
        // 1: acquire the head lock.
        std::unique_lock<std::mutex> lk(head.m);
        // we don't take ownership of the pointer to next, so we use next.get();
        while (node *const next = current->next.get()) {
            // if the next pointer is not nullPtr we obtain the lock on the node it points to
            std::unique_lock<std::mutex> next_l(next->m);
            // once we have the lock on the next node, we may release the previous lock
            lk.unlock();
            // apply the function to the data
            f(*next->data);
            // shift current pointer
            current = next;
            // move the ownership of the next node's lock to the lk
            lk = std::move(next_l);
        }
    }

    /**
     * Finds first element in the list that satisfies the condition of the Predicate.
     * If no element is found, it returns empty shared_ptr, i.e. nullPtr.
     * @tparam Predicate
     * @param p
     * @return
     */
    template<class Predicate>
    std::shared_ptr<T> find_first_if(Predicate p) {
        node *current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (node *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_l(next->m);
            lk.unlock();
            if (p(*next->data)) {
                return next->data;
            }
            current = next;
            lk = std::move(next_l);
        }
        return std::shared_ptr<T>();
    }
    /**
     * Removes all elements in the list that satisfy the condition of the Predicate.
     * @tparam Predicate
     * @param p
     */
    template<class Predicate>
    void remove_if(Predicate p) {
        node *current = &head;
        std::unique_lock<std::mutex> lk(head.m);
        while (node *const next = current->next.get()) {
            std::unique_lock<std::mutex> next_l(next->m);
            if (p(*next->data)) {
                std::unique_ptr<node> old_next = std::move(current->next);
                // simply make current node's next point to the next node's next
                // but still hold the lock of the current node
                current->next = std::move(next->next);
                next_l.unlock();
            } else {
                lk.unlock();
                current = next;
                lk = std::move(next_l);
            }
        }
    }
};





























