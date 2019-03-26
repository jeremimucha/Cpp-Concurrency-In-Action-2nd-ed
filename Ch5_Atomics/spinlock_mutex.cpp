#include <atomic>
#include <iostream>
#include <chrono>
#include <thread>

// std::atomic_flag is the simplest atomic type. It's guaranteed to be lock-free, but provides
// a very simple set of operations - `clear()` and `test_and_set()`.
// `clear()` is a store-type operation and can be
//   std::memory_order_relaxed,
//   std::memory_order_release,
//   std::memory_order_seq_cst,
// `test_and_set()` is a read-modify-write and in addition to the above can be
//   std::memory_order_acquire
//   std::memory_order_consume
//   std::memory_order_acq_rel
// It can be used to implement a simple spinlock-mutex.

class spinlock_mutex
{
public:
    spinlock_mutex() noexcept = default;

    void lock()
    {
        while (flag_.test_and_set(std::memory_order_acquire))
            ;
    }

    void unlock()
    {
        flag_.clear(std::memory_order_release);
    }

private:
// std::atomic_flag must always be initialized with ATOMIC_FLAG_INIT
// which clears the flag
    std::atomic_flag flag_{ATOMIC_FLAG_INIT};
};


int main()
{
    spinlock_mutex mutex{};
    auto const writer = [&mutex]{
        for (auto i{0}; i != 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds{150});
            mutex.lock();
            std::cerr << "Hello from thread[" << std::this_thread::get_id() << "]\n";
            mutex.unlock();
        }
    };

    auto t1 = std::thread{writer};
    auto t2 = std::thread{writer};
    auto t3 = std::thread{writer};
    t1.join();
    t2.join();
    t3.join();
}
