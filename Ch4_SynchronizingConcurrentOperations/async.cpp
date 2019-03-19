#include <iostream>
#include <future>
#include <thread>
#include <chrono>
#include <random>
#include <string>

/*
 * A program that demonstrates passing arguments to std::async and using different std::launch
 * policies with std::async
 */

namespace
{
std::default_random_engine re{std::chrono::system_clock::now().time_since_epoch().count()};
std::uniform_int_distribution<> ud(300,700);
}

struct Foo
{
    void abc(int count, std::string const& msg)
    {
        for (auto i{0}; i<count; ++i) {
            std::cout << "Foo::abc thread[" << std::this_thread::get_id()
                << "]: " << msg << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds{ud(re)});
        }
    }

    std::string def(std::string const& msg)
    {
        for (int i{0}; i<10; ++i) {
            std::cout << "Foo::def thread[" << std::this_thread::get_id()
                << "]: " << msg << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(ud(re)));
        }
        return "Hello from the future";
    }
};

struct Bar
{
    double operator()(double d)
    {
        for (int i{0}; i < static_cast<int>(d); ++i ) {
            std::cout << "Bar::operator() thread[" << std::this_thread::get_id()
                << "]: " << d << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(ud(re)));
        }
        return d;
    }
};

Foo baz(Foo& foo)
{
    std::this_thread::sleep_for(std::chrono::milliseconds{ud(re)});
    std::cout << "baz thread[" << std::this_thread::get_id()
        << "]" << std::endl;
    return foo;
}

int main()
{
// Passing arguments to std::async follows the same basic rules as passing arguments to std::thread
// does - arguments are copied, unless explicitly specified otherwise, and later moved
// within the implementation.

    Foo foo;
    // -- call Foo::abc on a pointer to foo
    auto f1 = std::async(&Foo::abc, &foo, 42, "Hello async world!");
    // -- call Foo::def on a copy of foo
    auto f2 = std::async(&Foo::def, foo, "Goodbye async world!");

    Bar bar;
    auto f3 = std::async(Bar(), 3.1415);    // Bar() is move constructed
    auto f4 = std::async(std::ref(bar), 2.718); // use a reference to bar
    
    // call baz on a reference to foo - note that you can ignore the result.
    std::async(baz, std::ref(foo));

    std::cout << "f2 result = " << f2.get() << "\n"
              << "f3 result = " << f3.get() << "\n"
              << "f4 result = " << f4.get()
              << std::endl;

// --- demonstrate std::launch policies
    auto async_action = []( int count){
        for(int i=0; i<count; ++i){
            std::cout << "thread[" << std::this_thread::get_id()
                << "] int = " << count << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(ud(re)));
        }
    };

    // -- std::launch::async - async must run in its own thread
    auto f5 = std::async(std::launch::async, async_action, 10);
    // -- std::launch::deferred - run when .wait() or .get() is called
    auto f6 = std::async(std::launch::deferred, async_action, 4);
    // -- std::launch::deferred | std::launch::async
    // implementation chooses - usually should run in its own thread if possible
    // this is the default behavior.
    auto f7 = std::async(std::launch::deferred | std::launch::async
                        ,async_action, 5);
    f6.wait();  // the deferred function is invoked at this point.

}
