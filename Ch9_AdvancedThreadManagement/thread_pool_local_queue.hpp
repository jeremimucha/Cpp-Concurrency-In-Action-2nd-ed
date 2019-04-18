#pragma once

#include <future>
#include <queue>
#include <thread>

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
        pool_work_queue_.push(std::move(task));
        return result;
    }

    void run_pending_task()
    {
        function_wrapper task;
        if (local_work_queue_ && !local_work_queue_->empty()) {
            task = std::move(local_work_queue_->front());
            local_work_queue_->pop();
            task();
        }
        if (pool_work_queue_.try_pop(task)) {
            task();
        }
        else {
            std::this_thread::yield();
        }
    }

private:
    void worker_thread()
    {
        local_work_queue_ = std::make_unique<local_queue_type>();

        while (!done_) {
            run_pending_task();
        }
    }

    // --- member data
    std::atomic_bool done_{false};
    threadsafe_queue<function_wrapper> pool_work_queue_{};
    std::vector<std::thread> threads_{};
    join_threads joiner_{threads_};
    using local_queue_type = std::queue<function_wrapper>;
    static thread_local std::unique_ptr<local_queue_type> local_work_queue_;
};

thread_local std::unique_ptr<thread_pool::local_queue_type> thread_pool::local_work_queue_{};
