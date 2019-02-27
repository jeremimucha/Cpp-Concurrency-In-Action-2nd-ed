#include <iostream>
#include <thread>
#include <functional>
#include <memory>
#include <utility>


struct Foo { };

void by_val(Foo) { std::cerr << __PRETTY_FUNCTION__ << "\n"; }
void by_ref(Foo&) { std::cerr << __PRETTY_FUNCTION__ << "\n"; }
void by_cref(Foo const&) { std::cerr << __PRETTY_FUNCTION__ << "\n"; }

struct task {
    explicit task(Foo& f) noexcept
        : ref_{f} { }

    template<typename T>
    void operator()(T&&) const { std::cerr << __PRETTY_FUNCTION__ << "\n"; }

    Foo& ref_;
};

void sink_uptr(std::unique_ptr<Foo>) { std::cerr << __PRETTY_FUNCTION__ << "\n"; }


int main()
{
// By default additional arguments are copied into internal std::thread storage,
// from which they're passed as rvalues into the callable
    Foo foo;
    // foo is copied into std::thread storage and moved into `by_value`
    std::thread by_val_thread{by_val, foo};
    by_val_thread.join();

    // The following STILL copies foo into thread storage, an rvalue is then passed to
    // the `by_cref` function, which is legal - rvalue can bind to const lvalue.
    std::thread by_cref_thread_1{by_cref, foo};
    by_cref_thread_1.join();

    // Passing to a `by_ref` function is illegal - rvalue can not bind to a non-const lvalue
    // std::thread by_ref_thread_err{by_ref, foo}; // doesn't compile
    // by_ref_thread_err.join();

    // To actually pass a reference std::ref or std::cref must be used
    std::thread actually_by_cref_thread{by_cref, std::cref(foo)};
    actually_by_cref_thread.join();

    // reference to foo is passed into thread storage and to the `by_ref` function
    std::thread by_ref_thread{by_ref, std::ref(foo)};
    by_ref_thread.join();

    // could also use a wrapper object
    std::thread wrapped_1{task{foo}, foo};
    std::thread wrapped_2{task{foo}, std::ref(foo)};
    std::thread wrapped_3{task{foo}, std::cref(foo)};
    wrapped_1.join();
    wrapped_2.join();
    wrapped_3.join();

    // or a lambda - corresponding to the above tasks
    std::thread lambda_3{[foo{foo}](){std::cerr << __PRETTY_FUNCTION__ << "\n"; }};
    std::thread lambda_1{[&foo=foo](){ std::cerr << __PRETTY_FUNCTION__ << "\n"; }};
    std::thread lambda_2{[&foo=foo]()mutable{ std::cerr << __PRETTY_FUNCTION__ << "\n"; }};
    lambda_1.join();
    lambda_2.join();
    lambda_3.join();

    // callables which take smart pointers can also be used
    std::thread sink_thread_1{sink_uptr, std::make_unique<Foo>()};
    auto uptr_foo{std::make_unique<Foo>()};
    // std::thread sink_thread_err{sink_uptr, uptr_foo};   // can't copy unique_ptr
    std::thread sink_thread_2{sink_uptr, std::move(uptr_foo)};
    sink_thread_1.join();
    sink_thread_2.join();
}
