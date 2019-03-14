#include <atomic>
#include <iostream>
#include <string>
#include <type_traits>
#include "threadsafe_stack.hpp"
#include <random>
#include <chrono>
#include <utility>

template<typename Stack, template<typename> class ReadPolicy, template<typename> class Handler>
class Reader : private ReadPolicy<Stack>, private Handler<Stack> {
    using ReadPolicy = typename ReadPolicy<Stack>;
    using Handler = typename Handler<Stack>;

public:
    explicit Reader(Stack& stack) noexcept
        : stack_{stack}
    {
    }

    void operator()()
    {
        auto&& value = ReadPolicy::read(stack_);
        Handler::process(std::forward<decltype(value)>(value));
    }

private:
    Stack& stack_;
};

// One way to read from the threadsafe stack is to get the value by an out-param
template<typename Stack>
class ReadByRef {
public:
    using return_type = typename Stack::value_type;

    static inline return_type read(Stack& stack)
    {
        return_type value{};    // requires default constructible
        stack.pop(value);
        return value;
    }
};

// Another way is to use the overload returning std::shared_ptr<T>
template<typename Stack>
class ReadSharedPtr {
public:
// Get the type returned by pop() overload taking no parameters - some shared_ptr
    using return_type = decltype(std::declval<Stack>().pop());

    static inline return_type read(Stack& stack)
    {
        return stack.pop();
    }
};

// A Handler could provide both interfaces - taking the type by value/reference/const_reference
// and another overload taking the shared_ptr - potentially just delegating the actual work
// to the by-ref overload
template<typename Stack>
class Handler {
public:
    using reference = std::add_lvalue_reference_t<
                            std::add_const_t<
                                typename Stack::value_type>>
    using pointer = decltype(std::declval<Stack>().pop());

    static inline void process(reference value)
    {
        std::cerr << __PRETTY_FUNCTION__ << "\n"
            << value << "\n";
    }

    static inline void process(pointer p_value)
    {
        std::cerr << __PRETTY_FUNCTION__ << "\n";
        process(*p_value);
    }
};

template<typename Stack>
class Writer {
public:
    explicit Writer(Stack& stack) noexcept
        : stack_{stack}
    {
    }

    void operator()()
    {
        std::ostringstream oss;
        oss << msg_ << std::to_string(++counter_) << " from Thread["
            << std::this_thread::get_id() << "]";
        stack_.push(std::move(oss).str());
    }

private:
    Stack& stack_;
    int counter_{};
    static constexpr inline char* const msg_{"Message #"};
};

template<typename Engine = std::default_random_engine,
         typename Distribution = std::uniform_int_distribution<>>
class RandomDispatcher {
public:
    using random_engine_type = Engine;
    using distribution_type = Distribution;
    using result_type = typename distribution_type::result_type;

    template<typename F>
    void operator()(F&& f)
    {
        // TODO: There should be yet another class that manages the atomic variable
        // for synchronization. It should dispatch the call on thread creation by passing
        // the atomic to the callable object. The callable should handle the atomic appropriately
        // - increment it on entry and decrement it on exit - if it's a Writer,
        // and spin on it if it's a reader
    }

private:
    static inline std::atomic_int_fast8_t active_writers_{};
};

int main()
{

}
