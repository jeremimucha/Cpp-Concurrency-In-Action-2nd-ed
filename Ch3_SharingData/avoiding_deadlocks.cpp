#include <iostream>
#include <thread>
#include <mutex>


/**
 * Deadlock-avoiding guidelines:
 * 1. Don't acquire a lock if you already hold one
 * 2. Avoid calling user supplied code while holding a lock
 * 3. If acquiring more than one lock is necessary - acquire them in a fixed order,
 *    preferably as a single operation - using std::lock or std::scoped_lock
 * 4. Use a lock hierarchy - hierarchical_mutex - to enforce locking order
 * 5. Don't wait on a thread if it might be waiting on you
 * 6. Don't wait on a thread while holding a lock
 */

class BigObject { };

// C++11/14 - using std::lock and std::lock_guard
class Foo {
public:
    Foo(BigObject obj)
        : data_{std::move(obj)}
    {
    }

    friend void swap(Foo& lhs, Foo& rhs) noexcept
    {
        // Trying to acquire a mutex while already holding it is undefined behavior,
        // ensure distinct objects
        if (&lhs == &rhs) {
            return;
        }

        std::lock(lhs.mutex_, rhs.mutex_);
        std::lock_guard<std::mutex> lock_lhs{lhs.mutex_, std::adopt_lock};
        std::lock_guard<std::mutex> lock_rhs{rhs.mutex_, std::adopt_lock};
        // assume optimized swap here
        auto&& temp = std::move(lhs.data_);
        lhs.data_ = std::move(rhs.data_);
        rhs.data_ = std::move(temp);
    }

private:
    BigObject data_{};
    std::mutex mutex_{};
};

// C++11/14 alternative using std::lock and std::unique_lock
class Bar {
public:
    Bar(BigObject obj)
        : data_{std::move(obj)}
    {
    }

    friend void swap(Bar& lhs, Bar& rhs) noexcept
    {
        // Trying to acquire a mutex while already holding it is undefined behavior,
        // ensure distinct objects
        if (&lhs == &rhs) {
            return;
        }

        std::unique_lock<std::mutex> lock_lhs{lhs.mutex_, std::defer_lock};
        std::unique_lock<std::mutex> lock_rhs{rhs.mutex_, std::defer_lock};
        std::lock(lhs.mutex_, rhs.mutex_);  // mutexes are locked here
        // assume optimized swap here
        auto&& temp = std::move(lhs.data_);
        lhs.data_ = std::move(rhs.data_);
        rhs.data_ = std::move(temp);
    }

private:
    BigObject data_{};
    std::mutex mutex_{};
};

// C++17 - using std::scoped_lock
class Baz {
public:
    Baz(BigObject obj)
        : data_{std::move(obj)}
    {
    }

    friend void swap(Baz& lhs, Baz& rhs) noexcept
    {
        // Trying to acquire a mutex while already holding it is undefined behavior,
        // ensure distinct objects
        if (&lhs == &rhs) {
            return;
        }

        std::scoped_lock lock{lhs.mutex_, rhs.mutex_};  // note the template deduction guides
        // assume optimized swap here
        auto&& temp = std::move(lhs.data_);
        lhs.data_ = std::move(rhs.data_);
        rhs.data_ = std::move(temp);
    }

private:
    BigObject data_{};
    std::mutex mutex_{};
};

int main()
{

}
