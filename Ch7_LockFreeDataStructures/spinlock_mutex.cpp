#include "spinlock_mutex.hpp"
#include <mutex>


int main()
{
    spinlock_mutex mutex{};
    auto const writer = [&mutex]{
        for (auto i{0}; i != 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds{150});
            // mutex.lock();
            std::lock_guard<spinlock_mutex> lk{mutex};
            std::cerr << "Hello from thread[" << std::this_thread::get_id() << "]\n";
            // mutex.unlock();
        }
    };

    auto t1 = std::thread{writer};
    auto t2 = std::thread{writer};
    auto t3 = std::thread{writer};
    t1.join();
    t2.join();
    t3.join();
}
