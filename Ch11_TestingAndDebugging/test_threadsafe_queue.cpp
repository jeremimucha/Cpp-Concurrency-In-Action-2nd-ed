#include <future>
#include <iostream>
#include <thread>
#include <utility>

#include "threadsafe_queue.hpp"

void test_concurrent_push_and_pop_on_empty_queue()
{
    threadsafe_queue<int> q;
    std::promise<void> go, push_ready, pop_ready;
    std::shared_future<void> ready{go.get_future()};
    std::future<void> push_done;
    std::future<int> pop_done;
    try {
        push_done = std::async(std::launch::async, [&q, &push_ready, ready{ready}] {
            push_ready.set_value();
            ready.wait();
            q.push(42);
        });
        pop_done = std::async(std::launch::async, [&q, &pop_ready, ready{ready}] {
            pop_ready.set_value();
            ready.wait();
            int value{0};
            q.wait_and_pop(value);
            return value;
        });
        push_ready.get_future().wait();
        pop_ready.get_future().wait();
        go.set_value();
        push_done.get();
        auto const val{pop_done.get()};
        std::cerr << "val == " << val << "\n";
        std::cerr << "q.empty() == " << std::boolalpha << q.empty() << "\n";
    }
    catch (...) {
        go.set_value();
        throw;
    }
}

int main()
{
    test_concurrent_push_and_pop_on_empty_queue();
}
