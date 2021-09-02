#pragma once

#include "message_base.h"

namespace messaging {
    class sender {
        queue *q; // sender is a wrapper around the queue pointer

    public:
        sender();

        // allow construction from pointer to queue
        explicit sender(queue *q_);

        template<class Message>
        void send(const Message &msg);
    };

    template<class Message>
    void sender::send(const Message &msg) {
        if (q) {
            q->push(msg);
        }
    }
}


