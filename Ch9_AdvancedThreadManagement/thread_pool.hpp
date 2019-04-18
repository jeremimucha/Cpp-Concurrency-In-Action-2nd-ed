#pragma once

#include <atomic>
#include <future>
#include <thread>
#include <vector>

#include "function_wrapper.hpp"
#include "join_threads.hpp"
#include "threadsafe_queue.hpp"

class thread_pool {
public:
    thread_pool()
    {
        auto const thread_count{std::thread::hardware_concurrency()};
        try {
            for (auto i{0u}; i != thread_count; ++i) {
                threads_.push_back(std::thread{&thread_pool::worker_thread, this});
            }
        }
        catch (...) {
            done_ = true;
            throw;
        }
    }

    ~thread_pool() noexcept { done_ = true; }

    template <typename Function>
    std::future<std::invoke_result_t<Function>> submit(Function&& f)
    {
        using result_type = std::invoke_result_t<Function>;
        std::packaged_task<result_type()> task{std::move(f)};
        auto result{task.get_future()};
        work_queue_.push(std::move(task));
        return result;
    }

private:
    void worker_thread()
    {
        while (!done_) {
            function_wrapper task;
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
    threadsafe_queue<function_wrapper> work_queue_{};
    std::vector<std::thread> threads_{};
    join_threads joiner_{threads_};
};
