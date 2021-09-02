#pragma once

#include "atm_messages.h"
#include "template_dispatcher.h"
#include "message_base.h"
#include "string"
#include "utility"

namespace messaging {
    // the message for closing the queue
    class close_queue {
    };

    class dispatcher {
        queue *q;
        bool chained;

        dispatcher(const dispatcher &) = delete;

        dispatcher &operator=(const dispatcher &) = delete;

        template<class Dispatcher, class Msg, class Func>
        friend
        class TemplateDispatcher; // allow TemplateDispatcherInstances to access internals
        void wait_and_dispatch() {
            for (;;) {
                auto msg = q->wait_and_pop();
                dispatch(msg);
            }
        }

        bool dispatch(const std::shared_ptr<message_base> &msg) {
            if (dynamic_cast<wrapped_message<close_queue> *>(msg.get())) {
                throw close_queue();
            }
            return false;
        }

    public:
        dispatcher(dispatcher &&other);

        explicit dispatcher(queue *q_);

        template<class Message, class Func>
        TemplateDispatcher<dispatcher, Message, Func>
        handle(Func &&f) {
            return TemplateDispatcher<dispatcher, Message, Func>(
                    q, this, std::forward<Func>(f)
            );
        }

        ~dispatcher() noexcept(false);

    };

}






















