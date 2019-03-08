#include <iostream>
#include <thread>
#include <mutex>


/**
 * Ensuing that a locked mutex is unlocked on scope exit can be done using std::lock_guard,
 * and since C++17 - std::scoped_lock - it can hold multiple locks at once. Pre C++17 it's
 * required to lock the mutexes using std::lock and then ensure proper mutex release by
 * holding each mutex manually using std::lock_gaurd{mutex, std::adopt_lock}.
 */


int main()
{
    std::mutex m;

// C++11/14
    auto t1 = std::thread{[&m]{
        std::lock_guard<std::mutex> lock{m};
        std::cerr << "Hello from pre-C++17!\n";
    }};

// C++17
    auto t2 = std::thread{[&m]{
        std::scoped_lock lock{m};
        std::cerr << "Hello from C++17!\n";
    }};

    t1.join();
    t2.join();
}
