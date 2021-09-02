#include "bank_machine.h"

bank_machine::bank_machine() : balance(199) {}

void bank_machine::done() {
    get_sender().send(messaging::close_queue());
}

void bank_machine::run() {
    try {
        for (;;) {
            incoming.wait()
                    .handle<verify_pin>(
                            [&](const verify_pin &msg) {
                                if (msg.pin == "1937") {
                                    msg.atm_queue.send(pin_verified());
                                } else {
                                    msg.atm_queue.send(pin_incorrect());
                                }
                            }
                    )
                    .handle<withdraw>(
                            [&](const withdraw &msg) {
                                if (balance >= msg.amount) {
                                    msg.atm_queue.send(withdraw_ok());
                                    balance -= msg.amount;
                                } else {
                                    msg.atm_queue.send(withdraw_denied());
                                }
                            }
                    )
                    .handle<get_balance>(
                            [&](const get_balance &msg) {
                                msg.atm_queue.send(::balance(balance));
                            }
                    )
                    .handle<withdrawal_processed>(
                            [&](const withdrawal_processed &msg) {
                            }
                    )
                    .handle<cancel_withdrawal>(
                            [&](const cancel_withdrawal &msg) {
                            }
                    );
        }
    } catch (const messaging::close_queue &ignored) {

    }
}

messaging::sender bank_machine::get_sender() {
    return incoming;
}