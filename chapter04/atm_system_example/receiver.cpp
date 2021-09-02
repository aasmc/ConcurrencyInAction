#include "receiver.h"

namespace messaging {
    dispatcher receiver::wait() {
        return dispatcher(&q);
    }
}