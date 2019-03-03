#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include "joining_thread.hpp"

/**
 * std::thread::hardware_concurrency returns a number of threads that can run truly concurrently
 * on a particular system. This information might be unavailable in which case zero is returned.
 */


int main()
{

    auto const thread_count = std::thread::hardware_concurrency();
    std::vector<joining_thread> threads{};
    std::mutex cerr_mutex;
    std::atomic_int count{thread_count != 0 ? thread_count : 2};
    std::generate_n(std::back_inserter(threads),
                count.load(),
                [&mtx=cerr_mutex, &count=count]{
                    return joining_thread{
                        [&mtx, &count]{
                            --count;
                            std::lock_guard lk{mtx};
                            std::cerr << "Hello from thread["
                                << std::this_thread::get_id() << "]\n";
                        }
                    };
                });
    while (count) {
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
    }
    std::lock_guard lk{cerr_mutex};
    std::cerr << "hardware concurrency = " << thread_count << "\n";
}
