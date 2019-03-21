#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <future>
#include <utility>
#include <chrono>

#include <threadsafe_queue.hpp>

template<typename TaskQueue, typename SynchPolicy>
class TaskSender
{
public:
    explicit TaskSender(TaskQueue& queue)
        : queue_{queue}
    {
    }

    // Not generic for brevity
    void operator()(int sleep)
    {
        // signal that a sender has started doing work
        // will signal work done on destruction
        auto const pin = SynchPolicy::pin_sender();
    
        for (auto i{0}; i!=10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds{sleep});
            std::ostringstream oss{"Task #"};
            oss << i << " from thread[" << std::this_thread::get_id() << "]";
            auto task = [&out_mutex=SynchPolicy::output_mutex(), msg{std::move(oss).str()}]{
                // cout synchronization not needed if only one sender
                using mutex_type = std::decay_t<decltype(out_mutex)>;
                std::lock_guard<mutex_type> lock{out_mutex};
                std::cerr << msg << "\n";
            }
            // synchronization handled by the threadsafe queue
            queue_.push(std::packaged_task<void()>{std::move(task)});
        }
    }

    TaskSender(TaskSender const&) = default;
    TaskSender(TaskSender&&) = default;
    TaskSender& operator=(TaskSender const&) = default;
    TaskSender& operator=(TaskSender&&) = default;
    ~TaskSender() noexcept = default;

private:
    TaskQueue& queue_;
};

template<typename TaskQueue, typename SynchPolicy>
class TaskProcessor
{
public:
    explicit TaskProcessor(TaskQueue& queue)
        : queue_{queue}
    {
    }

    void operator()()
    {
        while (SynchPolicy::start_flag() == false) {
            std::this_thread::yield();
        }
        while (!SynchPolicy::work_done()) {
            std::packaged_task<void()> task{};
            if (queue_.try_pop(task)) {
                task();
            }
        };
    }

    TaskProcessor(TaskProcessor const&) = default;
    TaskProcessor(TaskProcessor&&) = default;
    TaskProcessor& operator=(TaskProcessor const&) = default;
    TaskProcessor& operator=(TaskProcessor&&) = default;
    ~TaskProcessor() noexcept = default;

private:
    TaskQueue& queue_;
};

template<typename Atomic>
class AtomicPin
{
public:
    explicit AtomicPin(Atomic& atomic) noexcept
        : atomic_{atomic}
    {
        atomic_.fetch_add(1);
    }

    ~AtomicPin() noexcept
    {
        atomic_.fetch_sub(1);
    }

    AtomicPin(AtomicPin const&) = delete;
    AtomicPin(AtomicPin&&) = delete;
    AtomicPin& operator=(AtomicPin const&) = delete;
    AtomicPin& operator=(AtomicPin&&) = delete;

private:
    Atomic& atomic_;
};

class UntilAllSendersLive
{
public:
    using atomic_type = std::atomic_int;

    static inline AtomicPin<atomic_type> pin_sender() noexcept
    {
        return AtomicPin<atomic_type>{senders_alive_};
    }

    static inline bool start_flag() noexcept
    {
        // would be better to return a std::promise to use as a flag
        return senders_alive_.load() != 0;
    }

    static inline bool work_done() noexcept
    {
        return senders_alive_.load() == 0;
    }

    static inline std::mutex& output_mutex() noexcept
    {
        return output_mutex_;
    }

private:
    static inline atomic_type senders_alive_{0};
    static inline std::mutex output_mutex_{};
};

int main()
{
    using TaskQueue = threadsafe_queue<std::packaged_task<void()>>;
    TaskQueue task_queue{};

    using MessageSender = TaskSender<TaskQueue, UntilAllSendersLive>;
    using MessageProcessor = TaskProcessor<TaskQueue,  UntilAllSendersLive>;

    auto message_sender1 = std::thread{MessageSender{task_queue}, 123};
    auto message_sender2 = std::thread{MessageSender{task_queue}, 203};
    auto message_processor = std::thread{MessageProcessor{task_queue}};

    message_sender1.join();
    message_sender2.join();
    message_processor.join();
}
