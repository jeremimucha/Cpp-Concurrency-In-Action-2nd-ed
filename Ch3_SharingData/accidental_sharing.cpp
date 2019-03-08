#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <utility>

/**
 * - Don't pass pointers or references to protected data outside the scope of the lock,
 * - don't return pointers or references to protected data,
 * - don't store them in extrnally visible memory,
 * - don't pass them as arguments to user supplied functions,
 */

class SomeData {
public:
    SomeData(int i, std::string s)
        : i_{i}, str_{std::move(s)} { }
    void do_something() { std::cerr << __PRETTY_FUNCTION__ << "\n"; }

private:
    int i_{};
    std::string str_{};
};

class UnsafeDataWrapper {
public:
    explicit UnsafeDataWrapper(SomeData data)
        : data_{std::move(data)} { }

    template<typename Function>
    decltype(auto) process_data(Function&& f)
    {
        std::cerr << __PRETTY_FUNCTION__ << "\n";
        std::lock_guard<std::mutex> lock{m_};
        // "protected" data is passed to a user-supplied function - all bets are off at this point
        // the supplied function can do anything with the data including storing references
        // or pointers to it, making further access un-protected
        return std::forward<Function>(f)(data_);
    }

private:
    SomeData data_;
    mutable std::mutex m_{};
};

SomeData& malicious_function(SomeData& data)
{
    // return a naked reference to the data - making unprotected access possible
    return data;
}

template<typename T> struct type_info;

int main()
{
    UnsafeDataWrapper wrapped_data{SomeData{42, "oops"}};
    auto& data = wrapped_data.process_data(malicious_function);  // obtain a reference to the data
    data.do_something();    // unprotected access
}
