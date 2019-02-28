#include <chrono>
#include <iostream>
#include "scoped_thread.hpp"

struct func {
    constexpr explicit func(int v) noexcept
        : val_{v}
    {
    }

    int operator()() const noexcept
    {
        std::cerr << "func doing stuff with val == " << val_ << "\n";
        return val_;
    }

    int val_;
};

scoped_thread init_thread(int v)
{
    return scoped_thread{std::in_place, func{v}};
}

void do_stuff()
{
    std::cerr << "About to do some stuff that takes some time...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
}

int main()
{
    auto t = init_thread(42);
    do_stuff();
    // thread managed by scoped_thread object `t` will be joined at scope exit
}
