#include <atomic>
#include <iostream>
#include <string>
#include <type_traits>
#include <random>
#include <chrono>
#include <utility>

#include "threadsafe_stack.hpp"


template<typename Stack, template<typename> class ReadPolicy, template<typename> class Handler>
class Reader {
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
        if (!p_value) {
            throw std::runtime_error{"got nullptr value"};
        }
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


template<typename LoopPolicy,
         typename Engine = std::default_random_engine,
         typename Distribution = std::uniform_int_distribution<>>
class PeriodicAction {
public:
    using random_engine_type = Engine;
    using distribution_type = Distribution;
    using result_type = typename distribution_type::result_type;

    PeriodicAction(result_type min, result_type max)
        : distribution_{min, max}
    {
    }

    template<typename Action, typename... Args>
    void operator()(Action&& action, Args&&... args)
    {
        while (LoopPolicy condition{std::forward<Args>(args)...}; condition) {
            action();
        }
    }

private:
    distribution_type distribution_{};
    static inline random_engine_type re_{
        std::chrono::steady_clock::now().time_since_epoch().count()
    };
};

template<typename Atomic, unsigned N>
class BoundedLoopPolicy {
public:
    explicit BoundedLoopPolicy(Atomic& flag)
        : active_flag_{++flag}
    {
    }

    explicit operator bool() noexcept
    {
        if (++counter_ != N) {
            return true;
        }
        return false;
    }

    ~BoundedLoopPolicy() noexcept
    {
        --active_flag_;
    }
private:
    unsigned counter_{};
    Atomic& active_flag_;
};

template<typename Atomic, typename Stack>
class UntilEmptyLoopPolicy {
public:
    explicit UntilEmptyLoopPolicy(Atomic& flag, Stack const& stack)
        : stack_{stack}, active_flag_{flag}
    {
    }

    explicit operator bool() const noexcept
    {
        return active_flag_ != 0 && stack_.empty();
    }

private:
    Stack const& stack_;
    Atomic& active_flag_;
};

// class that manages the atomic variable
// for synchronization. It should dispatch the call on thread creation by passing
// the atomic to the callable object. The callable should handle the atomic appropriately
// - increment it on entry and decrement it on exit - if it's a Writer,
// and spin on it if it's a reader
class Dispatcher {
public:
    template<typename F>
    void operator()(F&& f)
    {
        std::forward<F>(f)(active_writers_);
    }

private:
    static inline std::atomic_int_fast8_t active_writers_{};
};

int main()
{

}
