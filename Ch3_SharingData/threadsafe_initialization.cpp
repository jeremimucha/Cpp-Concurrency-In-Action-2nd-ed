#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <memory>

struct Foo {
    void do_something() { std::cerr << __PRETTY_FUNCTION__ << "\n"; }
};

// Shared initialization using a std::mutex
// threadsafe but slow - all threads are synchronized at the mutex
std::shared_ptr<Foo> shared_resource_1;
std::mutex resource_mtx_1;
void init_using_mutex()
{
    std::unique_lock<std::mutex> lock{resource_mtx_1};
    if (!shared_resource_1) {
        shared_resource_1.reset(new Foo{});
    }
    lock.unlock();
    shared_resource_1->do_something();
}

// The double-checked locking pattern.
// Apparently this is not thread-safe - causes a data-race because the first
// check is not synchronized with the write
std::shared_ptr<Foo> shared_resource_2;
std::mutex resource_mtx_2;
void double_checked_locking_undefined_behavior()
{
    if (!shared_resource_2) {
        std::lock_guard<std::mutex> lock{resource_mtx_2};
        if (!shared_resource_2) {
            shared_resource_2.reset(new Foo{});
        }
    }
    shared_resource_2->do_something();  // data race - may be using invalid data here
}

// initialization using std::once_flag and std::call_once
std::shared_ptr<Foo> shared_resource_3;
std::once_flag resource_3_flag;
void init_using_call_once()
{
    std::call_once(resource_3_flag, []{shared_resource_3.reset(new Foo{});});
    shared_resource_3->do_something();
}

class LazyClass {
public:
    LazyClass() = default;

    void report_data()
    {
        std::call_once(init_flag_, &init_resource, this);
        std::cerr << "LazyClass thread[" << std::this_thread::get_id()
            << "] data = " << resource_->str() << "\n";
    }

    void update_data()
    {
        std::call_once(init_flag_, &init_resource, this);
        auto const id = std::this_thread::get_id();
        std::cerr << "LazyClass thread[" << id << "] updating data...\n";
        *resource_ << ", " << id;
    }

private:
    void init_resource() { resource_ = std::make_unique<std::ostringstream>(); }
    std::unique_ptr<std::ostringstream> resource_{};
    std::once_flag init_flag_{};
};

// Static local object initialization is thread safe - singletons can be safely initialized
// using the following pattern
class ThreadsafeSingleton {
public:
    static inline ThreadsafeSingleton& instance()
    {
        static ThreadsafeSingleton instance;
        return instance;
    }

private:
    ThreadsafeSingleton() = default;

    std::unique_ptr<Foo> resource_{std::make_unique<Foo>()};
};

int main()
{

}
