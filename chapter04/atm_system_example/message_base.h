#pragma once

#include "mutex"
#include "condition_variable"
#include "queue"
#include "memory"

namespace messaging {
    struct message_base {
        virtual ~message_base() {};
    };

    template<typename Msg>
    struct wrapped_message : message_base {
        Msg contents;

        explicit wrapped_message(const Msg &contents_) : contents(contents_) {};
    };

    class queue {
        std::mutex m;
        std::condition_variable c;
        /**
         * Internal queue stores pointers to message_base
         */
        std::queue<std::shared_ptr<message_base>> q;

    public:
        template<class T>
        void push(const T &msg) {
            std::lock_guard lk(m);
            // wrap posted message and store pointer
            q.push(std::make_shared<wrapped_message<T>>(msg));
            c.notify_all();
        }

        std::shared_ptr<message_base> wait_and_pop() {
            std::unique_lock lk(m);
            // block until queue is not empty, in other words, wait while the queue is empty
            c.wait(lk, [&] { return !q.empty(); });
            auto res = q.front();
            q.pop();
            return res;
        }
    };
}
































