cmake_minimum_required(VERSION 3.20)
project(ConcurrencyInAction)

set(APP_DOMAIN ./)
set(APP_ATM ./chapter04/atm_system_example)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(${APP_DOMAIN})
include_directories(${APP_ATM})

add_executable(ConcurrencyInAction chapter02/examples.cpp chapter03/thread_safe_stack.h chapter03/examples_ch03.cpp
        chapter03/hierarchical_mutex.h chapter04/thread_safe_queue.h chapter04/examples.cpp chapter04/quick_sort_examples.cpp
        chapter04/atm_system_example/message_base.h chapter04/atm_system_example/sender.h
        chapter04/atm_system_example/sender.cpp chapter04/atm_system_example/receiver.h chapter04/atm_system_example/receiver.cpp
        chapter04/atm_system_example/template_dispatcher.h chapter04/atm_system_example/dispatcher.h chapter04/atm_system_example/dispatcher.cpp
        chapter04/atm_system_example/atm_messages.h chapter04/atm_system_example/atm.h chapter04/atm_system_example/atm.cpp
        chapter04/atm_system_example/bank_machine.h chapter04/atm_system_example/bank_machine.cpp chapter04/atm_system_example/interface_machine.h
        chapter04/atm_system_example/interface_machine.cpp chapter04/atm_system_example/driver.cpp chapter05/spin_lock.h
        chapter05/example_memory_order_seq_cst.cpp chapter06_lock_based_data_structures/thread_safe_queue_revised.h chapter06_lock_based_data_structures/simple_queue.h chapter06_lock_based_data_structures/thread_safe_lookup_table.h chapter06_lock_based_data_structures/thread_safe_list.h chapter07_lock_free_data_structures/lock_free_stack.h chapter07_lock_free_data_structures/hazard_pointer.h)
