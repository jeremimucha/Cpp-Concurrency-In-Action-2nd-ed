#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "join_threads.hpp"
#include "threadsafe_queue.hpp"

class thread_pool {
public:
    thread_pool()
    {
        auto const thread_count{std::thread::hardware_concurrency()};
        try {
            for (auto i{0u}; i != thread_count; ++i) {
                threads_.push_back(
                    std::thread{&thread_pool::worker_thread, this});
            }
        }
        catch (...) {
            done_ = true;
            throw;
        }
    }

    ~thread_pool() noexcept { done_ = true; }

    template <typename Function>
    void submit(Function&& f)
    {
        work_queue_.push(std::function<void()>{std::forward<Function>(f)});
    }

private:
    void worker_thread()
    {
        while (!done_) {
            std::function<void()> task;
            if (work_queue_.try_pop(task)) {
                task();
            }
            else {
                std::this_thread::yield();
            }
        }
    }

    // --- member data
    std::atomic_bool done_{false};
    threadsafe_queue<std::function<void()>> work_queue_{};
    std::vector<std::thread> threads_{};
    join_threads joiner_{threads_};
};