#pragma once

#include <algorithm>
#include <future>
#include <thread>
#include <vector>

#include "join_threads.hpp"

template <typename Iterator, typename Function>
void parallel_for_each(Iterator begin, Iterator end, Function f)
{
    auto const length{std::distance(begin, end)};
    using size_type = decltype(length);
    if (0 == length) {
        return;
    }
    unsigned const min_per_thread{25u};
    size_type const max_threads{(length + min_per_thread - 1) / min_per_thread};
    unsigned const num_threads{std::min(
        std::max(std::thread::hardware_concurrency(), 2), max_threads)};
    size_type const block_size{length / num_threads};

    std::vector<std::future<void>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);
    join_threads joiner{threads};
    auto block_begin{begin};
    for (auto i{0u}; i < (num_threads - 1); ++i) {
        auto const block_end{std::next(block_begin, block_size)};
        std::packaged_task<void()> task{
            [b{block_begin}, e{block_end}, f] { std::for_each(b, e, f); }};
        futures[i] = task.get_future();
        threads[i] = std::thread{std::move(task)};
        block_start = block_end;
    }
    std::for_each(block_begin, end, f);
    // propagate possible exceptions
    for (auto& f : futures) {
        f.get();
    }
}
