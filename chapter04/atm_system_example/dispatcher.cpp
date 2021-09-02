#include "dispatcher.h"

namespace messaging {
    dispatcher::dispatcher(queue *q_) : q(q_), chained(false) {}

    dispatcher::dispatcher(dispatcher &&other) : q(other.q), chained(other.chained) {
        other.chained = true;
    }

    dispatcher::~dispatcher() noexcept(false) {
        if (!chained) {
            wait_and_dispatch();
        }
    }
}