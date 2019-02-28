#pragma once

#include <thread>
#include <type_traits>
#include <utility>

class scoped_thread {
public:
    explicit scoped_thread(std::thread t)
        : t_{std::move(t)}
    {
        if (!t_.joinable()) {
            throw std::logic_error("No thread!");
        }
    }

    template<typename... Args,
             typename = std::enable_if_t<std::is_constructible_v<std::thread, Args...>>>
    explicit scoped_thread(std::in_place_t, Args&&... args)
        noexcept(std::is_nothrow_constructible_v<std::thread, Args...>)
        : t_{std::forward<Args>(args)...}
    {
    }

    scoped_thread(scoped_thread const&) = delete;
    scoped_thread& operator=(scoped_thread const&) = delete;
    scoped_thread(scoped_thread&&) = default;
    scoped_thread& operator=(scoped_thread&&) = default;

    ~scoped_thread() noexcept
    {
        t_.join();
    }

private:
    std::thread t_{};
};