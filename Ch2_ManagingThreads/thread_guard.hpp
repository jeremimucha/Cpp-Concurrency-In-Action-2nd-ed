#pragma once

#include <thread>


class thread_guard {
public:
    constexpr explicit thread_guard(std::thread& t) noexcept
        : t_{t}
    {
    }

    ~thread_guard() noexcept 
    {
        if (t_.joinable()) {
            t_.join();
        }
    }

    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;

private:
    std::thread& t_;
};
