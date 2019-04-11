#ifndef THREADSAFE_STACK_HPP_
#define THREADSAFE_STACK_HPP_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

template <typename T>
class threadsafe_stack {
  private:
    struct node {
        std::unique_ptr<T> data{};
        std::unique_ptr<node> next{};

        node() = default;
        template <typename... Args,
            typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        node(Args&&... args)
            : data{std::make_unique<T>(std::forward<Args>(args)...)}
        {
        }
        node(T&& value)
            : data{std::make_unique<T>(std::move(value))}
        {
        }
        ~node() noexcept = default;
        node(node const&) = default;
        node(node&&) noexcept = default;
        node& operator=(node const&) = default;
        node& operator=(node&&) noexcept = default;
    };

    // --- member variables
    mutable std::mutex head_mutex_{};
    mutable std::condition_variable data_cond_{};
    node head{};
    // ---

    void push_node(std::unique_ptr<node>&& new_node)
    {
        {
            std::lock_guard<std::mutex> lk{head_mutex_};
            new_node->next = std::move(head.next);
            head.next = std::move(new_node);
        }
        data_cond_.notify_one();
    }

  public:
    using value_type = std::unique_ptr<T>;
    threadsafe_stack() = default;

    template <typename... Args>
    std::enable_if_t<std::is_constructible_v<node, Args...>>
    emplace(Args&&... args)
    {
        auto new_node{std::make_unique<node>(std::forward<Args>(args)...)};
        push_node(std::move(new_node));
    }

    void push(T&& value)
    {
        auto new_node{std::make_unique<node>(std::move(value))};
        push_node(std::move(new_node));
    }

    void push(T const& value)
    {
        auto new_node{std::make_unique<node>(value)};
        push_node(std::move(new_node));
    }

    std::unique_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk{head_mutex_};
        data_cond_.wait(lk, [this] { return head.next != nullptr; });
        auto old_head = std::move(head.next);
        head.next = std::move(old_head->next);
        return std::move(old_head->data);
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(head_mutex_);
        data_cond_.wait(lk, [this] { return head.next != nullptr; });
        auto old_head = std::move(head.next);
        head.next = std::move(old_head->next);
        lk.unlock();
        value = std::move(*old_head->data);
    }

    std::unique_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk{head_mutex_};
        if (head.next == nullptr) {
            return nullptr;
        }
        auto old_head = std::move(head.next);
        head.next = std::move(old_head->next);
        return std::move(old_head->data);
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk{head_mutex_};
        if (head.next == nullptr) {
            return false;
        }
        auto old_head = std::move(head.next);
        head.next = std::move(old_head->next);
        value = std::move(*old_head->data);
        return true;
    }

    bool empty() const noexcept
    {
        std::lock_guard<std::mutex> lk{head_mutex_};
        return head.next == nullptr;
    }
};

#endif /* THREADSAFE_STACK_HPP_ */
