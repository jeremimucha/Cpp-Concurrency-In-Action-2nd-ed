#include <iostream>
#include <thread>
#include <future>


int main()
{
    std::promise<void> sync{};
    auto flag = sync.get_future();
    auto world = [flag{std::move(flag)}]() mutable {
        flag.wait();
        std::cerr << ", World";
    };
    auto hello = [sync{std::move(sync)}]() mutable {
        std::cerr << "Hello";
        sync.set_value();
    };

    // example using std::async does not work! Bug or flawed logic?
    // std::async(std::launch::async, std::move(world));    // ignore the returned future
    auto t1 = std::thread{std::move(world)};
    auto t2 = std::thread{std::move(hello)};
    t1.join();
    // auto f {std::async(std::launch::async, std::move(world))};

    std::cerr << " of Concurrency!\n";

    t2.join();
}
