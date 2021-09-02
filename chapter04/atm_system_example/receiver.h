#pragma once

#include "sender.h"
#include "dispatcher.h"

namespace messaging {
    class receiver {
        queue q; // a receiver owns the queue

    public:
        // allow implicit conversion to a sender that references the queue
        operator sender() {
            return sender(&q);
        }
        // waiting for a queue creates a dispatcher
        dispatcher wait();
    };
}
