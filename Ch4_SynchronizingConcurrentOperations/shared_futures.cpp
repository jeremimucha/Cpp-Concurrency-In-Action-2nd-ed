#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <string>
#include <chrono>
#include <utility>
/*
 * std::shared_future is used to share a future (state) between threads.
 * it can be created from a std::future object. Accessing a single
 * std::shared_future object from multiple threads requires external
 * synchronization (via mutex), however accessing the shared result from
 * copies of std::shared_future concurrently is safe.
 */


void signal_shared()
{
// --- Using std::shared_future for signalling ready-state to multiple threads
    std::promise<void> ready_promise, t1_ready_promise, t2_ready_promise;
    // one way to construct an std::shared_future
    std::shared_future<void> ready_future(ready_promise.get_future());

    // std::chrono::time_point<std::chrono::steady_clock> start;
    auto start = std::chrono::steady_clock::now();

    auto t1_ready_future{t1_ready_promise.get_future()};
    auto fun1 = [promise{std::move(t1_ready_promise)}, start, ready_future]() mutable
    {
        promise.set_value();   // make the promise ready
        ready_future.wait();    // waits for signal from main()
        return std::chrono::steady_clock::now() - start;
    };

    auto t2_ready_future{t2_ready_promise.get_future()};
    auto fun2 = [promise{std::move(t2_ready_promise)}, start, ready_future]() mutable
    {
        promise.set_value();
        ready_future.wait();    // wait for the signal from main()
        return std::chrono::steady_clock::now() - start;
    };

    auto result1 = std::async(std::launch::async, std::move(fun1));
    auto result2 = std::async(std::launch::async, std::move(fun2));

    // wait for the threads to become ready
    t1_ready_future.wait();
    t2_ready_future.wait();

    // the threads are ready, start the clock
    start = std::chrono::steady_clock::now();

    // signal the threads to go
    ready_promise.set_value();

    std::cout << "Thread 1 received the signal "
              << result1.get().count() << " ms after start\n"
              << "Thread 2 received the signal "
              << result2.get().count() << " ms after start\n";
}

std::mutex cout_mutex;

void share_result()
{
    std::packaged_task<std::string(int,int)> task{
        [](int ticks, int duration) {
            for( int i=0; i<ticks; ++i ){
                std::cout << "Task on thread[" << std::this_thread::get_id()
                    << "] doing work... " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds{duration});
            }
            return std::string{"Hello std::shared_future!"};
        }};

    // another way to construct an std::shared_future from a std::future object
    auto shared_result{task.get_future().share()};

    auto fun = [shared_result]()
        {
            std::cout << "Thread[" << std::this_thread::get_id() << "] started"
                << " waiting for shared_result..." << std::endl;
            auto result = shared_result.get();
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "Thread[" << std::this_thread::get_id()
                << "] got result = " << result << std::endl;
        };

    auto res1{std::async(fun)};
    auto res2{std::async(fun)};

    std::thread t{std::move(task), 10, 351};

    t.join();
    std::lock_guard<std::mutex> lk{cout_mutex};
    std::cout << "main got result = " << shared_result.get();
}


int main()
{
    signal_shared();
    share_result();
}
