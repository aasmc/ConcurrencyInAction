#pragma once

#include "message_base.h"

namespace messaging {
    template<typename PreviousDispatcher, typename Msg, typename Func>
    class TemplateDispatcher {
        queue *q;
        PreviousDispatcher *prev;
        Func f;
        bool chained;

        TemplateDispatcher(const TemplateDispatcher &) = delete;

        TemplateDispatcher &operator=(const TemplateDispatcher &) = delete;

        template<class Dispatcher, class OtherMsg, class OtherFunc>
        friend
        class TemplateDispatcher; // TemplateDispatcher instantiations are friends of each other
        void wait_and_dispatch() {
            for (;;) {
                auto msg = q->wait_and_pop();
                if (dispatch(msg)) {
                    break; // break out of the loop if you handle the message
                }
            }
        }

        bool dispatch(const std::shared_ptr<message_base> &msg) {
            // check the message type and call the function
            if (wrapped_message<Msg> *wrapper = dynamic_cast<wrapped_message<Msg> *>(msg.get())) {
                f(wrapper->contents);
                return true;
            } else {
                return prev->dispatch(msg); // chain to the previous dispatcher
            }
        }

    public:
        TemplateDispatcher(TemplateDispatcher &&other) :
                q(other.q), prev(other.prev), f(std::move(other.f)), chained(other.chained) {
            other.chained = true;
        }

        TemplateDispatcher(queue *q_, PreviousDispatcher *prev_, Func &&f_) :
                q(q_), prev(prev_), f(std::move(f_)), chained(false) {
            prev_->chained = true;
        }

        template<class OtherMsg, class OtherFunc>
        TemplateDispatcher<TemplateDispatcher, OtherMsg, OtherFunc>
        handle(OtherFunc &&of) { // additional handlers can be chained together
            return TemplateDispatcher<TemplateDispatcher, OtherMsg, OtherFunc>(q, this, std::forward<OtherFunc>(of));
        }

        ~TemplateDispatcher() noexcept(false) {
            if (!chained) {
                wait_and_dispatch(); // any of the handlers from the method might throw an exception, so mark the destructor as noexcept(false)
            }
        }
    };
}






























