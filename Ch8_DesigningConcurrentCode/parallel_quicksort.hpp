#pragma once

#include <algorithm>
#include <atomic>
#include <future>
#include <list>
#include <thread>
#include <vector>
#include <type_traits>

#include "joining_thread.hpp"
#include "threadsafe_stack.hpp"

template <typename T>
struct sorter {
    struct chunk_to_sort {
        std::list<T> data{};
        std::promise<std::list<T>> promise{};
    };

    // --- member data
    threadsafe_stack<chunk_to_sort> chunks{};
    std::vector<std::thread> threads{};
    unsigned const max_thread_count{max_threads() - 1};
    std::atomic_bool end_of_data{false};
    // ---

    static constexpr inline unsigned max_threads() noexcept
    {
        auto const hw_concurrency = std::thread::hardware_concurrency();
        return std::max(hw_concurrency, 2u);
    }

    sorter() = default;

    ~sorter() noexcept
    {
        end_of_data = true;
        for (auto&& t : threads) {
            t.join();
        }
    }

    void try_sort_chunk()
    {
        auto const p_chunk = chunks.try_pop();
        if (p_chunk) {
            sort_chunk(*p_chunk);
        }
    }

    std::list<T> do_sort(std::list<T>& chunk_data)
    {
        if (chunk_data.empty()) {
            return chunk_data;
        }
        std::list<T> result;
        result.splice(result.begin(), chunk_data, chunk_data.begin());
        auto const div_point = std::partition(chunk_data.begin(), chunk_data.end(),
                                              [part_val{result.front()}](auto const& val) { return val < part_val; });
        chunk_to_sort new_lower_chunk;
        new_lower_chunk.data.splice(new_lower_chunk.data.end(),
                                    chunk_data, chunk_data.begin(),
                                    div_point);
        auto lower_future = new_lower_chunk.promise.get_future();
        chunks.push(std::move(new_lower_chunk));
        if (threads.size() < max_thread_count) {
            threads.push_back(std::thread{&sorter<T>::sort_thread, this});
        }

        std::list<T> new_higher{do_sort(chunk_data)};
        result.splice(result.end(), new_higher);
        while (lower_future.wait_for(std::chrono::milliseconds{0}) != std::future_status::ready) {
            try_sort_chunk();
        }
        result.splice(result.begin(), lower_future.get());
        return result;
    }

    void sort_chunk(chunk_to_sort& chunk)
    {
        chunk.promise.set_value(do_sort(chunk.data));
    }

    void sort_thread()
    {
        while (!end_of_data) {
            try_sort_chunk();
            std::this_thread::yield();
        }
    }
};

template <typename T>
std::list<T> parallel_quicksort(std::list<T> input)
{
    if (input.empty()) {
        return input;
    }
    sorter<T> s{};
    return s.do_sort(input);
}
