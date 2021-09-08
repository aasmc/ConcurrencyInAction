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

    node head;

public:
    thread_safe_list() {}

    ~thread_safe_list() {
        remove_if([](const node &) { return true; })
    }

    thread_safe_list(const thread_safe_list &other)=delete;
    thread_safe_list& operator=(const thread_safe_list &other)=delete;

    template<class Predicate>
    void remove_if(Predicate p) {

    }
};





























