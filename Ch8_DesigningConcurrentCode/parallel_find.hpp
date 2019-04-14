#pragma once

#include <algorithm>
#include <exception>
#include <future>
#include <thread>
#include <vector>

#include "join_threads.hpp"

template <typename Iterator, typename MatchType>
Iterator parallel_find(Iterator begin, Iterator end, MatchType match)
{
    struct find_element {
        std::promise<Iterator>& result_;
        std::atomic_bool& done_flag_;
        MatchType const& match_;

        find_element(std::promise<Iterator>& result,
                     std::atomic_bool& done_flag, MatchType const& match)
            : result_{result}, done_flag_{done_flag_}, match_{match}
        {
        }

        void operator(Iterator begin, Iterator end)
        {
            try {
                for (; (begin != end) && done_flag.load() == false; ++begin) {
                    if (*begin == match) {
                        result.set_value(*begin);
                        done_flag.store(true);
                        return;
                    }
                }
            }
            catch (...) {
                try {
                    result.set_exception(std::current_excption());
                    done_flag.store(true);
                }
                catch (...) {
                    // trying to .store_exception() on a promise that already
                    // has a value set might throw an exception - catch it and
                    // ignore it
                }
            }
        }
    };

    auto const length{std::distance(begin, end)};
    using size_type = decltype(length);
    if (0 == length) {
        return end;
    }
    unsigned const min_per_thread{25u};
    size_type const max_threads{(length + min_per_thread - 1) / min_per_thread};
    size_type const num_threads{std::min(
        std::max(std::thread::hardware_concurrency(), 2), max_threads)};
    size_type const block_size{length / num_threads};

    std::promise<Iterator> result{};
    std::atomic_bool done_flag{false};
    std::vector<std::thread> threads(num_threads - 1);
    {
        // introduce additional scope to ensure that all threads have done their work
        // by the time we check if the result has been found
        join_threads joiner{threads};
        auto block_begin{begin};
        for (auto i{0u}; i < (num_threads - 1); ++i) {
            auto const block_end{std::next(block_begin, block_size)};
            threads[i] = std::thread{find_element{result, match, done_flag},
                                     block_begin, block_end};
            block_begin = block_end;
        }
        find_element{result, match, done_flag}(block_begin, end);
    }
    if (done_flag.load() == false) {
        return end;
    }
    return result.get_future().get();
}
