#include <iostream>
#include <string>
#include <future>
#include <chrono>

int main()
{
    // std::promise<int> promise;
    // auto f = promise.get_future();
    // auto f2 = f.then([](std::future<int> future){
    //     std::cerr << "Got answer: " << future.get();
    //     return "Foobar";
    // });
    // std::thread t{[p{std::move(promise)}]()mutable{
    //     std::cerr << "thread[" << std::this_thread::get_id() << "] started\n";
    //     std::this_thread::sleep_for(std::chrono::milliseconds{123});
    //     p.set_value(42);
    // }};

    // std::cerr << "f2 returned: " << f2.get() << "\n";
}
