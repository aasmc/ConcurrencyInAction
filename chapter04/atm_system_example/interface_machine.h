#pragma once

#include "receiver.h"

class interface_machine {
    messaging::receiver incoming;
public:
    void done();

    void run();

    messaging::sender get_sender();
};
