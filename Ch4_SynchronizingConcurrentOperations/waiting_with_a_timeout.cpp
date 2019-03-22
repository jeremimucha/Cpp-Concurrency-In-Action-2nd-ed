#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>


class Waiter
{
public:
    explicit Waiter(std::atomic_bool& done, std::condition_variable& cv, std::mutex& m)
        : done_{done}, cv_{cv}, m_{m}
    {
    }

    Waiter(Waiter&&) noexcept = default;
    Waiter& operator=(Waiter&&) = default;

    void operator()(std::chrono::milliseconds timeout)
    {
    // Recommended way of waiting with a time limit, if a predicate isn't being passed to `wait`.
    // This way the total length of the loop is bounded, even in the face of spurious wake ups
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        std::unique_lock<std::mutex> lock{m_};
        while(!done_.load() && cv_.wait_until(lock, deadline) != std::cv_status::timeout) {
            std::cerr << "thread[" << std::this_thread::get_id() << "] waiting...\n";
        }
        std::cerr << "thread[" << std::this_thread::get_id() << "] done with flag == "
            << done_.load() << "\n";
    }

private:
    std::atomic_bool& done_;
    std::condition_variable& cv_;
    std::mutex& m_;
};


int main()
{
    std::atomic_bool flag{false};
    std::condition_variable cv{};
    std::mutex m{};
    auto waiter1 = std::thread{Waiter{flag, cv, m}, std::chrono::milliseconds{1000}};
    auto waiter2 = std::thread{Waiter{flag, cv, m}, std::chrono::milliseconds{300}};
    auto signal = std::thread{[&flag]()mutable{
            std::this_thread::sleep_for(std::chrono::milliseconds{734});
            flag.store(true);
        }};

    waiter2.join();
    signal.join();
    waiter1.join();
}
