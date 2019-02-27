#include <iostream>
#include <thread>
#include <chrono>
#include "thread_guard.hpp"


void do_something_in_main_thread(int state)
{
    std::cerr << "\nlocal_state in thread [" << std::this_thread::get_id()
        << "] = " << state << "\n";
}

int main()
{
    int local_state{0};
    std::thread t{
        [&state=local_state]()
        {
            for(auto i{1u}; i != 42; ++i) {
                std::cerr << ++state << " ";
            }
        }
    };

    thread_guard gurad{t};
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    // this lacks locking - so the output might be mangled
    do_something_in_main_thread(local_state);
    // but still thread `t` will be properly join'ed on scope exit
    // even if the `do_something_in_local_thread` function throws.
}
