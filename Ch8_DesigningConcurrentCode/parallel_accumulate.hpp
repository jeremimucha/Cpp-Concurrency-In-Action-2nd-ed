#pragma once

#include <algorithm>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "join_threads.hpp"

template <typename Iterator, typename T>
T parallel_accumulate(Iterator begin, Iterator end, T init)
{
    auto const length{std::distance(begin, end)};
    if (length == 0) {
        return init;
    }
    unsigned const min_per_thread{25u};
    unsigned const max_threads{(length + min_per_thread - 1) / min_per_thread};
    unsigned const num_threads{std::min(
        std::max(std::thread::hardware_concurrency(), 2), max_threads)};
    unsigned const block_size{length / num_threads};
    std::vector<std::future<T>> futures(num_threads - 1);
    std::vector<std::thread> threads(num_threads - 1);
    join_threads joiner{threads};

    auto block_begin{begin};
    for (auto i{0u}; i < (num_threads - 1); ++i) {
        auto block_end{block_begin};
        std::advance(block_end, block_size);
        std::packaged_task<T(Iterator, Iterator)> task{
            [](Iterator b, Iterator e) { return std::accumulate(b, e, T{}); }};
        futures[i] = task.get_future();
        threads[i] = std::thread{std::move(task), block_begin, block_end};
        block_begin = block_end;
    }
    auto const last_result{std::accumulate(block_begin, end, T{})};
    auto result{init};
    for (auto& f : futures) {
        result += f.get();
    }
    result += last_result;
    return result;
}
