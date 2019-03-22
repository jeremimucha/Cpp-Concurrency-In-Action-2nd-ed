#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <memory>
#include <type_traits>

template<typename T>
class threadsafe_queue
{
public:
    threadsafe_queue() = default;

    threadsafe_queue(threadsafe_queue const& other)
        : data_{std::lock_guard<std::mutex>{other.mtx_}, other.data_}
    {
    }

    threadsafe_queue(threadsafe_queue&& other) noexcept(
        std::is_nothrow_move_constructible_v<std::queue<T>>)
        : data_{std::lock_guard<std::mutex>{other.mtx_}, std::move(other.data_)}
    {
    }

    threadsafe_queue& operator=(threadsafe_queue const& other)
    {
        if (this == &other) {
            return *this;
        }
        {
            std::lock(mtx_, other.mtx_);
            std::lock_guard<std::mutex> lk_this{mtx_, std::adopt_lock};
            std::lock_guard<std::mutex> lk_other{other.mtx_, std::adopt_lock};
            data_ = other.data_;
        }
        return *this;
    }

    threadsafe_queue& operator=(threadsafe_queue&& other) noexcept(
        std::is_move_assignable_v<std::queue<T>>)
    {
        if (this == &other) {
            return *this;
        }
        {
            std::lock(mtx_, other.mtx_);
            std::lock_guard<std::mutex> lk_this{mtx_, std::adopt_lock};
            std::lock_guard<std::mutex> lk_other{other.mtx_, std::adopt_lock};
            data_ = other.data_;
        }
        return *this;
    }

    void push(T value)
    {
        std::lock_guard<std::mutex> lock{mtx_};
        data_.push(std::move(value));
        data_cond_.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lock{mtx_};
        data_cond_.wait([this]{return !data_.empty();});
        value = std::move(data_).front();
        data_.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lock{mtx_};
        data_cond_.wait(lock, [this]{ return !data_.empty(); });
        auto const res{std::make_shared<T>(std::move(data_.front()))};
        data_.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lock{mtx_};
        if (data_.empty()) {
            return false;
        }
        value = std::move(data_.front());
        data_.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lock{mtx_};
        if (data_.empty()) {
            return {nullptr};
        }
        auto const res{std::make_shared<T>(std::move(data_.front()))};
        data_.pop();
        return res;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock{mtx_};
        return data_.empty();
    }

private:
    mutable std::mutex mtx_{};
    std::queue<T> data_{};
    std::condition_variable data_cond_{};
};