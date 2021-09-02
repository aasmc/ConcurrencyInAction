#pragma once

#include "receiver.h"

class bank_machine {
    messaging::receiver incoming;
    unsigned balance;

public:
    bank_machine();
    void done();
    void run();

    messaging::sender get_sender();
};
