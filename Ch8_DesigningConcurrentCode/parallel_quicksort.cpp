#include <algorithm>
#include <iostream>
#include <string>
#include <list>
#include <random>

namespace
{
std::default_random_engine re{12345u};
std::uniform_int_distribution<> ud{0, 1234};
} // namespace


#include "parallel_quicksort.hpp"

template<typename T>
std::ostream& operator<<(std::ostream& os, std::list<T> const& data)
{
    for (auto const& e : data) {
        os << e << "\n";
    }
    return os;
}

int main()
{
    std::list<std::string> data;
    std::generate_n(std::back_inserter(data), 10, [](){
        return "Hello parallel world! #" + std::to_string(ud(re));
    });
    std::cerr << "pre sort:\n";
    std::cerr << data;

    auto const sorted = parallel_quicksort(std::move(data));
    std::cerr << "sorted:\n";
    std::cerr << sorted;
}
