#if !defined(THREADSAFE_QUEUE_HPP_)
#define THREADSAFE_QUEUE_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

template <typename T>
class threadsafe_queue {
  private:
    struct node {
        std::shared_ptr<T> data{nullptr};
        std::unique_ptr<node> next{nullptr};
    };

    // --- member data
    std::unique_ptr<node> head_{std::make_unique<node>()};
    node* tail_{head_.get()};
    mutable std::mutex head_mutex_{};
    mutable std::mutex tail_mutex_{};
    std::condition_variable cv_{};

    // helper functions
    node* get_tail();
    std::unique_ptr<node> pop_head();
    std::unique_lock<std::mutex> wait_for_data();
    template <typename Clock>
    std::pair<std::unique_lock<std::mutex>, bool>
    wait_for_data_until(std::chrono::time_point<Clock> deadline);
    std::unique_ptr<node> wait_pop_head();
    std::unique_ptr<node> wait_pop_head(T& value);
    template <typename Clock>
    std::unique_ptr<node> wait_pop_head_until(std::chrono::time_point<Clock> deadline);
    template <typename Clock>
    bool wait_pop_head_until(T& value, std::chrono::time_point<Clock> deadline);
    std::unique_ptr<node> try_pop_head();
    std::unique_ptr<node> try_pop_head(T& value);
    void push_new_data(std::shared_ptr<T> new_data);

  public:
    threadsafe_queue() = default;
    threadsafe_queue(threadsafe_queue const&) = delete;
    threadsafe_queue(threadsafe_queue&&) noexcept = delete;
    threadsafe_queue& operator=(threadsafe_queue const&) = delete;
    threadsafe_queue&& operator=(threadsafe_queue&&) = delete;
    ~threadsafe_queue() noexcept = default;

    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);
    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T& value);
    template <typename Clock>
    std::shared_ptr<T> wait_and_pop_until(std::chrono::time_point<Clock> deadline);
    template <typename Clock>
    bool wait_and_pop_until(T& value, std::chrono::time_point<Clock> deadline);
    void push(T new_value);
    template <typename... Args>
    std::enable_if_t<std::is_constructible_v<T, Args...>> emplace(Args&&... args);
    bool empty() const;
};

// helper functions
template <typename T>
auto threadsafe_queue<T>::pop_head() -> std::unique_ptr<node>
{
    auto old_head{std::move(head_)};
    head_ = std::move(old_head->next);
    return old_head;
}

template <typename T>
auto threadsafe_queue<T>::get_tail() -> node*
{
    std::lock_guard<std::mutex> lock{tail_mutex_};
    return tail_;
}

template <typename T>
std::unique_lock<std::mutex> threadsafe_queue<T>::wait_for_data()
{
    std::unique_lock<std::mutex> lock{head_mutex_};
    cv_.wait(lock, [this] { return head_.get() != get_tail(); });
    return lock; // might need std::move(lock) pre C++17
}

template <typename T>
template <typename Clock>
std::pair<std::unique_lock<std::mutex>, bool>
threadsafe_queue<T>::wait_for_data_until(std::chrono::time_point<Clock> deadline)
{
    std::unique_lock<std::mutex> lock{head_mutex_};
    auto const res{cv_.wait_until(lock, deadline, [this] { return head_.get() != get_tail(); })};
    return {std::move(lock), res};
}

template <typename T>
auto threadsafe_queue<T>::wait_pop_head() -> std::unique_ptr<node>
{
    std::unique_lock<std::mutex> lock{wait_for_data()};
    return pop_head();
}

template <typename T>
auto threadsafe_queue<T>::wait_pop_head(T& value) -> std::unique_ptr<node>
{
    std::unique_lock<std::mutex> lock{wait_for_data()};
    auto const pdata = std::move(head_->data);
    auto old_head = pop_head();
    lock.unlock();
    value = std::move(*pdata);
    return old_head;
}

template <typename T>
template <typename Clock>
auto threadsafe_queue<T>::wait_pop_head_until(std::chrono::time_point<Clock> deadline)
    -> std::unique_ptr<node>
{
    auto [lock, data_available]{wait_for_data_until(deadline)};
    if (!data_available) {
        return nullptr;
    }
    return pop_head();
}

template <typename T>
template <typename Clock>
bool threadsafe_queue<T>::wait_pop_head_until(T& value, std::chrono::time_point<Clock> deadline)
{
    auto [lock, data_available]{wait_for_data_until(deadline)};
    if (!data_available) {
        return false;
    }
    auto const pdata = std::move(head_->data);
    pop_head();
    lock.unlock();
    value = std::move(*pdata);
    return true;
}

template <typename T>
auto threadsafe_queue<T>::try_pop_head() -> std::unique_ptr<node>
{
    std::lock_guard<std::mutex> lock{head_mutex_};
    if (head_.get() == get_tail()) {
        return {};
    }
    return pop_head();
}

template <typename T>
auto threadsafe_queue<T>::try_pop_head(T& value) -> std::unique_ptr<node>
{
    std::unique_lock<std::mutex> lock{head_mutex_};
    if (head_.get() == get_tail()) {
        return {};
    }
    auto const pdata = std::move(head_->data);
    auto old_head = pop_head();
    lock.unlock();
    value = std::move(*pdata);
    return old_head;
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
    auto const old_head{try_pop_head()};
    if (nullptr == old_head) {
        return {};
    }
    return old_head->data;
}

template <typename T>
bool threadsafe_queue<T>::try_pop(T& value)
{
    auto const old_head{try_pop_head(value)};
    return nullptr != old_head;
}

template <typename T>
bool threadsafe_queue<T>::empty() const
{
    std::lock_guard<std::mutex> lock{head_mutex_};
    return head_.get() == get_tail();
}

template <typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()
{
    auto const old_head{wait_pop_head()};
    return std::move(old_head->data);
}

template <typename T>
void threadsafe_queue<T>::wait_and_pop(T& value)
{
    auto const old_head{wait_pop_head(value)};
}

template <typename T>
template <typename Clock>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop_until(std::chrono::time_point<Clock> deadline)
{
    auto const old_head{wait_pop_head_until(deadline)};
    if (nullptr != old_head) {
        return std::move(old_head->data);
    }
    return nullptr;
}

template <typename T>
template <typename Clock>
bool threadsafe_queue<T>::wait_and_pop_until(T& value, std::chrono::time_point<Clock> deadline)
{
    return wait_pop_head_until(value, deadline);
}

template <typename T>
void threadsafe_queue<T>::push_new_data(std::shared_ptr<T> new_data)
{
    auto p{std::make_unique<node>()};
    {
        std::lock_guard<std::mutex> lock{tail_mutex_};
        tail_->data = std::move(new_data);
        node* const new_tail{p.get()};
        tail_->next = std::move(p);
        tail_ = new_tail;
    }
    cv_.notify_one();
}

template <typename T>
void threadsafe_queue<T>::push(T new_value)
{
    push_new_data(std::make_shared<T>(std::move(new_value)));
}

template <typename T>
template <typename... Args>
std::enable_if_t<std::is_constructible_v<T, Args...>>
threadsafe_queue<T>::emplace(Args&&... args)
{
    push_new_data(std::make_shared<T>(std::forward<Args>(args)...));
}

#endif // THREADSAFE_QUEUE_HPP_
