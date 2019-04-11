#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

template <typename T>
class threadsafe_list {
  private:
    struct debug_delete {
        template <typename U>
        void operator()(U* obj) noexcept
        {
            std::cerr << "Deleting object " << reinterpret_cast<void*>(obj)
                      << " value: " << *obj->data "\n";
            delete obj;
        }
    };

    struct node {
        std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node, debug_delete> next{nullptr, debug_delete{}};

        node() = default;
        template <typename... Args,
                  typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
        node(Args&&... args)
            : data{std::make_shared<T>(std::forward<Args>(args)...)}
        {
        }
        node(node const&) = delete;
        node& operator=(node const&) = delete;
        node(node&&) noexcept = default;
        node& operator=(node&&) noexcept = default;
        ~node() noexcept = default;
    };

    // --- member variables
    node head;

    void push_node(std::unique_ptr<node, debug_delete>&& new_node)
    {
        std::lock_guard<std::mutex> lk{head.m};
        new_node->next = std::move{head.next};
        head.next = std::move(new_node);
    }

  public:
    threadsafe_list() = default;
    ~threadsafe_list() noexcept
    {
        remove_if([](const node&) { return true; });
    }

    threadsafe_list(const threadsafe_list&) = delete;
    threadsafe_list& operator=(const threadsafe_list&) = delete;

    template <typename... Args>
    std::enable_if_t<std::is_constructible_v<node, Args...>>
    emplace_front(Args&&... args)
    {
        std::unique_ptr<node, debug_delete> new_node{new node{std::forward(args)...}, debug_delete{}};
        push_node(std::move(new_node));
    }

    void push_front(const T& value)
    {
        std::unique_ptr<node, debug_delete> new_node{new node{value}, debug_delete()};
        push_node(std::move(new_node));
    }

    template <typename Function>
    void for_each(Function&& func)
    {
        node* current = &head;
        std::unique_lock<std::mutex> lk{head.m};
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock(); // we unlock this as soon as possible
            std::forward<Function>(func)(*next->data);
            current = next;
            lk = std::move(next_lk);
        }
    }

    template <typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate pred)
    {
        node* current = &head;
        // need to lock the head before we access the node it points to
        std::unique_lock<std::mutex> lk{head.m};
        while (node* const next = current->next.get()) {
            // lock the next node before accessing the data it points to
            std::unique_lock<std::mutex> next_lk{next->m};
            // unlock this mutex before calling user-supplied code
            lk.unlock();
            if (pred(*next->data)) {
                return next->data;
            }
            current = next;
            // transfer lock ownership so that we keep the (now) current node locked
            lk = std::move(next_lk);
        }
        return nullptr;
    }

    template <typename Predicate>
    void remove_if(Predicate p)
    {
        node* current = &head;
        std::unique_lock<std::mutex> lk{head.m};
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk{next->m};
            if (p(*next->data)) {
                // transfer ownership into this scope - old_next will be destroyed
                // on scope exit
                std::unique_ptr<node, debug_delete> const old_next{std::move(current->next)};
                current->next = std::move(next->next);
                // need to unlock this before old_next is destroyed
                // destroying a locked mutex is undefined behavior
                next_lk.unlock();
            }
            else {
                // otherwise just iterate
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
};
