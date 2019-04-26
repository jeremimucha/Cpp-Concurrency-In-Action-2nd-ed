#include <future>
#include <iostream>
#include <string>
#include <thread>

#include "even_start.hpp"
#include "threadsafe_queue.hpp"

int main()
{
    auto results{
        even_start([] { return false; }, [] { return true; }, [] { return "foo"; })};
    std::cerr << "get<0> = " << std::boolalpha << std::get<0>(results).get() << "\n";
    std::cerr << "get<1> = " << std::boolalpha << std::get<1>(results).get() << "\n";
    std::cerr << "get<2> = " << std::get<2>(results).get() << "\n";

    threadsafe_queue<int> queue{};
    auto const push_action{[&q=queue]{q.push(42); return true;}};
    auto const pop_action{[&q=queue](){
        int value{0};
        q.wait_and_pop(value);
        return value;
    }};
    auto q_results{even_start(push_action, pop_action)};
    std::cerr << "pop result = " << std::get<1>(q_results).get() << "\n";
}
