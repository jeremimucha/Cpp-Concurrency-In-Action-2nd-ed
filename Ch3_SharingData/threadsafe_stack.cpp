#include <atomic>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <random>
#include <chrono>
#include <utility>
#include <thread>
#include <type_traits>

#include "threadsafe_stack.hpp"


template<typename Stack, template<typename> class ReadPolicy, template<typename> class Handler>
class Reader {
    using read_policy = ReadPolicy<Stack>;
    using handler_policy = Handler<Stack>;

public:
    explicit Reader(Stack& stack) noexcept
        : stack_{stack}
    {
    }

    void operator()()
    {
        auto value = read_policy::read(stack_);
        handler_policy::process(value);
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
class ReportingHandler {
public:
    using reference = std::add_lvalue_reference_t<
                            std::add_const_t<
                                typename Stack::value_type>>;
    using pointer = decltype(std::declval<Stack>().pop());

    // requires operator<<(std::ostream&, reference)
    static inline void process(reference value)
    {
        std::cerr << "ReportingHandler [" << std::this_thread::get_id() << "]: "
            << value << "\n";
    }

    static inline void process(pointer p_value)
    {
        std::cerr << __FUNCTION__ << ": ";
        if (!p_value) {
            throw std::runtime_error{"got nullptr value"};
        }
        process(*p_value);
    }
};

// Note - not very generic. Maybe it doesn't need to be, given other components?
// This would be just a concrete implementation of a writer for a concrete threadsafe_stack
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
        stack_.push(oss.str());
        std::cerr << "Pushed message: " << std::move(oss).str() << "\n";
    }

private:
    Stack& stack_;
    int counter_{};
    static inline constexpr char const* const msg_{"Message #"};
};

template<typename Stack>
Writer(Stack& stack) -> Writer<Stack>;

template<typename Action,
         typename Engine = std::default_random_engine,
         typename Distribution = std::uniform_int_distribution<>>
class PeriodicAction {
public:
    using random_engine_type = Engine;
    using distribution_type = Distribution;
    using result_type = typename distribution_type::result_type;

    PeriodicAction(Action action, result_type min, result_type max)
        : action_{std::move(action)}, distribution_{min, max}
    {
    }

    void operator()()
    {
        action_();
        std::this_thread::sleep_for(std::chrono::milliseconds{distribution_(engine_)});
    }

private:
    Action action_;
    distribution_type distribution_{};
    static inline random_engine_type engine_{
        static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())
    };
};

template<unsigned N, typename Atomic>
class BoundedLoopPolicy {
public:
    explicit BoundedLoopPolicy(Atomic& flag)
        : active_flag_{flag}
    {
        ++active_flag_;
        std::cerr << "BoundedLoopPolicy ctor, flag = " << active_flag_.load() << "\n";
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
        std::cerr << "BoundedLoopPolicy dtor, flag = " << active_flag_.load() << "\n";
    }
private:
    unsigned counter_{};
    Atomic& active_flag_;
};

template<typename Stack, typename Atomic>
class UntilEmptyLoopPolicy {
public:
    explicit UntilEmptyLoopPolicy(Atomic& flag, Stack const& stack)
        : stack_{stack}, active_flag_{flag}
    {
    }

    explicit operator bool() const noexcept
    {
        return active_flag_.load() != 0 || !stack_.empty();
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
// using atomic_type = std::atomic_int;
template<typename DispatchPolicy>
class Dispatcher {
public:
    using atomic_type = std::atomic_int;

    template<typename... Args
            ,typename = std::enable_if_t<
                            std::is_constructible_v<
                                DispatchPolicy,
                                std::add_lvalue_reference_t<atomic_type>,
                                Args...>>>
    explicit Dispatcher(std::atomic_int& active_writers, Args&&... args)
        : dispatch_condition_{active_writers, std::forward<Args>(args)...}
    {
        std::cerr << "Dispatcher ctor\n";
    }

    template<typename Action>
    void operator()(Action&& action)
    {
        while (dispatch_condition_) {
            std::forward<Action>(action)();
        }
    }

private:
    DispatchPolicy dispatch_condition_;
};


template<typename Stack, template<typename> class ReadPolicy>
using ReportingReader = Reader<Stack, ReadPolicy, ReportingHandler>;

int main()
{
    using StackType = threadsafe_stack<std::string>;
    using StackWriter = Writer<StackType>;
    using StackRefReader = ReportingReader<StackType, ReadByRef>;
    using StackPtrReader = ReportingReader<StackType, ReadSharedPtr>;

    StackType stack{};

    std::atomic_int writer_threads_{0};
    std::cerr << "0) active writer threads = " << writer_threads_.load();

// Writer is wrapped in a lambda here, because it seems like std::thread copy constructs the
// target thread object at least three times, at the same time - the dtor is called more often
// than the ctor, leading to a negative atomic flag value.
// Alternatively - BoundedLoopPolicy should implement proper copy controll.
    auto const writer_task = [&writer_threads_](auto&& arg){
            Dispatcher<BoundedLoopPolicy<10, std::atomic_int>> dispatcher{writer_threads_};
            dispatcher(std::forward<decltype(arg)>(arg));
        };
    auto writer1 = std::thread{
            writer_task,
            PeriodicAction<StackWriter>{StackWriter{stack}, 200, 456}
        };
    auto writer2 = std::thread{
            writer_task,
            PeriodicAction<StackWriter>{StackWriter{stack}, 100, 897}
        };
    
// No need to wrap reader in a lambda - nothing is done in the ctor/dtor
    Dispatcher<UntilEmptyLoopPolicy<StackType, std::atomic_int>> reader_task{writer_threads_, stack};
    auto reader1 = std::thread{
            reader_task,
            PeriodicAction<StackRefReader>{StackRefReader{stack}, 234, 888}
        };
    auto reader2 = std::thread{
            reader_task,
            PeriodicAction<StackPtrReader>{StackPtrReader{stack}, 123, 987}
        };

// Proper synchronization on thread-start is missing - as is one of the reader threads
// could start reading before anything is written to the stack - causing an exception to be thrown
// (due to this particular stack implementation) - this could be avoided by using a different
// stack (implementing a waiting pop operation or a try-an-pop) or by forcing the reader threads
// to start after the writer threads start writing
    writer1.join();
    writer2.join();
    reader1.join();
    reader2.join();
}
