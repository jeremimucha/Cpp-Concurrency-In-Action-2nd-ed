#include <iostream>
#include <future>
#include <list>
#include <algorithm>
#include <random>


template<typename T>
std::list<T> parallel_quicksort(std::list<T> input)
{
    if (input.empty()) {
        return input;
    }
    std::list<T> result{};
    result.splice(std::cbegin(result), input, std::cbegin(input));
    auto const& pivot = result.front();
    auto const divide_point = std::partition(std::begin(input), std::end(input),
                                [&pivot](auto const& e){ return e < pivot; }
                              );
    std::list<T> lower;
    lower.splice(lower.cend(), input, input.cbegin(), divide_point);
    std::future<std::list<T>> new_lower = std::async(&parallel_quicksort<T>, std::move(lower));
    auto new_higher = parallel_quicksort<T>(std::move(input));
    result.splice(result.cend(), new_higher);
    result.splice(result.cbegin(), new_lower.get());
    return result;
}


template<typename Engine = std::default_random_engine,
         typename Distribution = std::uniform_int_distribution<>
        >
class Rng {
public:
    using random_engine_type = Engine;
    using distribution_type = Distribution;
    using result_type = typename distribution_type::result_type;

    Rng() = default;
    Rng(result_type min, result_type max)
        : ud_{min, max}
    {
    }

    result_type operator()() noexcept { return ud_(re_); }

private:
    static inline std::default_random_engine re_{
        static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())
    };
    std::uniform_int_distribution<> ud_{500, 2000};
};

template<typename T>
std::ostream& operator<<(std::ostream& os, std::list<T> const& lst)
{
    for (auto it{std::cbegin(lst)}; it != std::cend(lst); ++it) {
        os << *it << ", ";
    }
    return os;
}

int main()
{
    std::list<int> data;
    std::generate_n(std::back_inserter(data), 25, Rng{0, 111});
    std::cerr << "Pre-sort data: " << data << "\n";
    auto const sorted{parallel_quicksort(data)};
    std::cerr << "sorted data: " << sorted << "\n";

}