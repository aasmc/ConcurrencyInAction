#include "interface_machine.h"
#include "iostream"
#include "mutex"

std::mutex iom;

messaging::sender interface_machine::get_sender() {
    return incoming;
}

void interface_machine::done() {
    get_sender().send(messaging::close_queue());
}

void interface_machine::run() {
    try {
        for (;;) {
            incoming.wait()
                    .handle<issue_money>(
                            [&](const issue_money &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Issuing "
                                              << msg.amount << std::endl;
                                }
                            }
                    )
                    .handle<display_insufficient_funds>(
                            [&](const display_insufficient_funds &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Insufficient funds" << std::endl;
                                }
                            }
                    )
                    .handle<display_enter_pin>(
                            [&](const display_enter_pin &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Please enter your PIN (0-9) " << std::endl;
                                }
                            }
                    )
                    .handle<display_enter_card>(
                            [&](const display_enter_card &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Please enter your card (I)"
                                              << std::endl;
                                }
                            }
                    )
                    .handle<display_balance>(
                            [&](const display_balance &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout
                                            << "The balance of your account is "
                                            << msg.amount << std::endl;
                                }
                            }
                    )
                    .handle<display_withdrawal_options>(
                            [&](const display_withdrawal_options &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Withdraw 50? (w)" << std::endl;
                                    std::cout << "Display Balance? (b)"
                                              << std::endl;
                                    std::cout << "Cancel? (c)" << std::endl;
                                }
                            }
                    )
                    .handle<display_withdrawal_cancelled>(
                            [&](const display_withdrawal_cancelled &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Withdrawal cancelled"
                                              << std::endl;
                                }
                            }
                    )
                    .handle<display_pin_incorrect_message>(
                            [&](const display_pin_incorrect_message &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "PIN incorrect" << std::endl;
                                }
                            }
                    )
                    .handle<eject_card>(
                            [&](const eject_card &msg) {
                                {
                                    std::lock_guard<std::mutex> lk(iom);
                                    std::cout << "Ejecting card" << std::endl;
                                }
                            }
                    );
        }
    } catch (const messaging::close_queue &ignored) {

    }
}