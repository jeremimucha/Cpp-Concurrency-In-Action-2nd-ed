#include <algorithm>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "thread_pool.hpp"

template <typename Iterator, typename T>
T parallel_accumulate(Iterator begin, Iterator end, T init)
{
    auto const length{std::distance(begin, end)};
    if (length == 0) {
        return init;
    }
    auto const block_size{25u};
    auto const num_blocks{(length + block_size - 1) / block_size};
    std::vector<std::future<T>> futures(num_blocks - 1);
    thread_pool pool{};
    auto block_begin{begin};
    for (auto i{0u}; i != (num_blocks - 1); ++i) {
        auto const block_end{std::next(block_begin, block_size)};
        futures[i] = pool.submit([b{block_begin}, e{block_end}] {
            return std::accumulate(b, e, T{});
        });
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

int main()
{
    std::vector<double> data;
    std::generate_n(std::back_inserter(data), 100, [d{3.14}]() mutable noexcept {
        constexpr double e{2.71828};
        d *= e;
        return d;
    });
    auto const result{parallel_accumulate(data.cbegin(), data.cend(), 0.0)};
    std::cerr << "result = " << result << "\n";
}
