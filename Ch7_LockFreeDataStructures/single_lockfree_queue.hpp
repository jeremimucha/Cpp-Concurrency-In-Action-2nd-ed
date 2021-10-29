#pragma once
#include <atomic>
#include <memory>
#include <utility>
#include <type_traits>

// single-producer, single-consumer atomic queue


template<typename T>
class lock_free_queue {
private:
    struct node {
        std::shared_ptr<T> data{nullptr};
        node* next;
    };

    std::atomic<node*> head_{new node};
    std::atomic<node*> tail_{head_.load()};

    node* pop_head()
    {
        node* const old_head = head_.load();    // #1

        if (old_head == tail.load())
        {
            return nullptr;
        }

        head_.store(old_head->next);
        return old_head;
    }

public:
    lock_free_queue(lock_free_queue const&) = delete;
    lock_free_queue& operator=(lock_free_queue const&) = delete;

    ~lock_free_queue()
    {
        while (node* const old_head = head_.load())
        {
            head_.store(old_head->next);
            delete old_head;
        }
    }

    std::shared_ptr<T> pop()
    {
        node* old_head = pop_head();
        if (!old_head)
        {
            return {nullptr};
        }

        std::shared_ptr<T> res{old_head->data}; // #2
        delete old_head;
        return res;
    }

    void push(T new_value)
    {
        auto new_data{std::make_shared<T>(new_value)};
        node* p = new node;
        node* const old_tail = tail_.load(); // sequenced-before load from the `data` pointr #2
        old_tail->data.swap(new_data);
        old_tail->next = p; // sequenced-before store to tail_
        tail_.store(p); // synchronizes with the load in pop_head #1
    }
};
