#pragma once

#include <exception>
#include <memory>
#include <mutex>
#include <stack>
#include <type_traits>

/**
 * threadsafe_stack examples a simplest implementation of a stack that can be used by concurrent
 * code. It no longer provides the split `top()` and `pop()` operations that a single-thread stack
 * does - `pop()` now returns the poped value to avoid a race condition - either by an out-param
 * or via a shared_ptr. Alternatively `pop()` could return by value, but it'd be safe only if
 * the stored type can be copied or moved without throwing.
 */

struct empty_stack : public std::exception {
    using std::exception::exception;
    using std::exception::what;
};

template<typename T>
class threadsafe_stack {
public:
    using value_type = typename std::stack<T>::value_type;

    threadsafe_stack() = default;

    threadsafe_stack(threadsafe_stack const& other)
    {
        std::lock_guard<std::mutex> lock{other.m_};
        data_ = other.data_;
    }

    threadsafe_stack(threadsafe_stack&& other) noexcept
        : data_{std::lock_guard<std::mutex>{other.m_}, std::move(other.data_)}
    {
    }

    threadsafe_stack& operator=(threadsafe_stack&& other) = delete;
    threadsafe_stack& operator=(threadsafe_stack const&) = delete;

    ~threadsafe_stack() noexcept = default;

    void push(T new_value) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        std::lock_guard<std::mutex> lock{m_};
        data_.push(std::move(new_value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock{m_};
        if (data_.empty()) {
            throw empty_stack{};
        }
        auto const res{std::make_shared<T>(data_.top())};
        data_.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock{m_};
        if (data_.empty()) {
            throw empty_stack{};
        }
        value = std::move(data_).top();
        // value = data_.top();
        data_.pop();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock{m_};
        return data_.empty();
    }

private:
    std::stack<T> data_{};
    mutable std::mutex m_{};
};
