#pragma once

#include "atomic"
#include "memory"
#include "thread"
#include "stdexcept"
#include "functional"

unsigned const max_hazard_pointers = 100;
struct hazard_pointer {
    std::atomic<std::thread::id> id;
    std::atomic<void *> pointer;
};

hazard_pointer hazard_pointers[max_hazard_pointers];

class hp_owner {
    hazard_pointer *hp;

public:
    hp_owner(const hp_owner &) = delete;

    hp_owner operator=(const hp_owner &) = delete;

    hp_owner() : hp(nullptr) {
        for (unsigned i = 0; i < max_hazard_pointers; ++i) {
            std::thread::id old_id;
            if (hazard_pointers[i].id.compare_exchange_strong(
                    old_id, std::this_thread::get_id()
                    )) {
                hp = &hazard_pointers[i];
                break;
            }
        }
        if (!hp) {
            throw std::runtime_error("No hazard pointers available");
        }
    }

    std::atomic<void *> &get_pointer() {
        return hp->pointer;
    }

    ~hp_owner() {
        hp->pointer.store(nullptr);
        hp->id.store(std::thread::id);
    }
};

/**
 * The first time each thread calls this function, a new instance of hp_owner
 * is created. The constructor if this new instance then searches through the table
 * of owner | pointer pairs looking for an entry without an owner. It uses
 * compare_exchange_strong to check for an entry without an owner and claim it in one go.
 * If the compare_exchange_strong() fails, another thread owns that entry, so
 * you move on to the next. If the exchange succeeds, you’ve successfully claimed the
 * entry for the current thread, so you store it and stop the search d . If you get to the
 * end of the list without finding a free entry e , there are too many threads using hazard
 * pointers, so you throw an exception.
 * @return
 */
std::atomic<void *> &get_hazard_pointer_for_current_thread() {
    // each thread has its own hazard pointer
    thread_local static hp_owner hazard;
    return hazard.get_pointer();
}

bool outstanding_hazard_pointers_for(void *p) {
    for (unsigned i = 0; i < max_hazard_pointers; ++i) {
        if (hazard_pointers[i].pointer.load() == p) {
            return true;
        }
    }
    return false;
}

template<typename T>
void do_delete(void *p) {
    // delete can handle only real pointer types not void* so that's why use static_cast
    delete static_cast<T *>(p)
}

struct data_to_reclaim {
    void *data;
    std::function<void < (void * )> deleter;
    data_to_reclaim *next;

    template<class T>
            data_to_reclaim(T *p) :
            data(p),
            deleter(&do_delete<T>),
            next(nullptr) {};

    ~data_to_reclaim() {
        deleter(data)
    }
};

std::atomic<data_to_reclaim *> nodes_to_reclaim;

void add_to_reclaim_list(data_to_reclaim *node) {
    node->next = nodes_to_reclaim.load();
    while (!nodes_to_reclaim.compare_exchange_weak(node->next, node));
}

template<typename T>
void reclaim_later(T *data) {
    add_to_reclaim_list(new data_to_reclaim(data));
}

void delete_nodes_with_no_hazards() {
    // This simple but crucial step ensures that this is the only thread trying
    // to reclaim this particular set of nodes.
    data_to_reclaim *current = nodes_to_reclaim.exchange(nullptr);
    while (current) {
        data_to_reclaim *const next = current->next;
        if (!outstanding_hazard_pointers_for(current->data)) {
            delete current;
        } else {
            add_to_reclaim_list(current);
        }
        current = next;
    }
}

