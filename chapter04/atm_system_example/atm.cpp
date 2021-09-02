#include "atm.h"

void atm::process_withdrawal() {
    incoming.wait()
            .handle<withdraw_ok>(
                    [&](const withdraw_ok &msg) {
                        interface_hardware.send(
                                issue_money(withdrawal_amount));
                        bank.send(
                                withdrawal_processed(account, withdrawal_amount));
                        state = &atm::done_processing;
                    }
            )
            .handle<withdraw_denied>(
                    [&](const withdraw_denied &msg) {
                        interface_hardware.send(display_insufficient_funds());
                        state = &atm::done_processing;
                    }
            )
            .handle<cancel_pressed>(
                    [&](const cancel_pressed &msg) {
                        bank.send(
                                cancel_withdrawal(account, withdrawal_amount));
                        interface_hardware.send(
                                display_withdrawal_cancelled());
                        state = &atm::done_processing;
                    });
}

void atm::process_balance() {
    incoming.wait()
            .handle<balance>(
                    [&](const balance &msg) {
                        interface_hardware.send(display_balance(msg.amount));
                        state = &atm::wait_for_action;
                    }
            )
            .handle<cancel_pressed>(
                    [&](const cancel_pressed &msg) {
                        state = &atm::done_processing;
                    }
            );
}

void atm::wait_for_action() {
    interface_hardware.send(display_withdrawal_options());
    incoming.wait()
            .handle<withdraw_pressed>(
                    [&](const withdraw_pressed &msg) {
                        withdrawal_amount = msg.amount;
                        bank.send(withdraw(account, msg.amount, incoming));
                        state = &atm::process_withdrawal;
                    }
            )
            .handle<balance_pressed>(
                    [&](const balance_pressed &msg) {
                        bank.send(get_balance(account, incoming));
                        state = &atm::process_balance;
                    }
            )
            .handle<cancel_pressed>(
                    [&](const cancel_pressed &msg) {
                        state = &atm::done_processing;
                    }
            );
}

void atm::verifying_pin() {

    incoming.wait()
            .handle<pin_verified>(
                    [&](const pin_verified &msg) {
                        state = &atm::wait_for_action;
                    }
            )
            .handle<pin_incorrect>(
                    [&](pin_incorrect const &msg) {
                        interface_hardware.send(
                                display_pin_incorrect_message());
                        state = &atm::done_processing;
                    }
            )
            .handle<cancel_pressed>(
                    [&](cancel_pressed const &msg) {
                        state = &atm::done_processing;
                    }
            );
}


void atm::getting_pin() {
    incoming.wait()
            .handle<digit_pressed>(
                    [&](const digit_pressed &msg) {
                        unsigned const pin_length = 4;
                        pin += msg.digit;
                        if (pin.length() == pin_length) {
                            bank.send(verify_pin(account, pin, incoming));
                            state = &atm::verifying_pin;
                        }
                    }
            )
            .handle<clear_last_pressed>(
                    [&](const clear_last_pressed &msg) {
                        if (!pin.empty()) {
                            pin.pop_back();
                        }
                    }
            )
            .handle<cancel_pressed>(
                    [&](const cancel_pressed &msg) {
                        state = &atm::done_processing;
                    }
            );
}

void atm::waiting_for_card() {
    interface_hardware.send(display_enter_card());
    incoming.wait()
            .handle<card_inserted>(
                    [&](const card_inserted &msg) {
                        account = msg.account;
                        pin = "";
                        interface_hardware.send(display_enter_pin());
                        state = &atm::getting_pin;
                    });
}

void atm::done_processing() {
    interface_hardware.send(eject_card());
    state = &atm::waiting_for_card;
}

atm::atm(messaging::sender bank_, messaging::sender interface_hardware_)
        : bank(bank_), interface_hardware(interface_hardware_) {};

void atm::done() {
    get_sender().send(messaging::close_queue());
}

void atm::run() {
    state = &atm::waiting_for_card;
    try {
        for (;;) {
            (this->*state)();
        }
    }
    catch (const messaging::close_queue &) {
    }
}

messaging::sender atm::get_sender() {
    return incoming;
}

























