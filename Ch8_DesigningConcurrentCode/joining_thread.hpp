#pragma once

#include <thread>
#include <type_traits>
#include <utility>


class joining_thread {
public:
    joining_thread() noexcept = default;

    template<typename Function, typename... Args,
             typename = std::enable_if_t<std::is_constructible_v<std::thread, Function, Args...>>>
    explicit joining_thread(Function&& function, Args&&... args)
        : t_{std::forward<Function>(function), std::forward<Args>(args)...}
    {
    }

    explicit joining_thread(std::thread t) noexcept
        : t_{std::move(t)}
    {
    }

    joining_thread(joining_thread const&) = delete;
    joining_thread& operator=(joining_thread const&) = delete;

    explicit joining_thread(joining_thread&& other) noexcept
        : t_{std::move(other.t_)}
    {
    }

    joining_thread& operator=(joining_thread&& other) noexcept
    {
        if (t_.joinable()) {
            t_.join();
        }
        t_ = std::move(other.t_);
        return *this;
    }

    joining_thread& operator=(std::thread other) noexcept
    {
        if (t_.joinable()) {
            t_.join();
        }
        t_ = std::move(other);
        return *this;
    }

    ~joining_thread() noexcept
    {
        if (t_.joinable()) {
            t_.join();
        }
    }

    void swap(joining_thread& other) noexcept
    {
        using std::swap;
        swap(t_, other.t_);
    }

    std::thread::id get_id() const noexcept
    {
        return t_.get_id();
    }

    bool joinable() const noexcept
    {
        return t_.joinable();
    }

    void join() { t_.join(); }

    void detach() { t_.detach(); }

    std::thread& as_thread() noexcept { return t_; }
    std::thread const& as_thread() const noexcept { return t_; }

private:
    std::thread t_{};
};
