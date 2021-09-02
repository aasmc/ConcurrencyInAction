#include "sender.h"

namespace messaging {
    sender::sender() : q(nullptr) {}

    sender::sender(queue *q_) : q(q_) {}
}