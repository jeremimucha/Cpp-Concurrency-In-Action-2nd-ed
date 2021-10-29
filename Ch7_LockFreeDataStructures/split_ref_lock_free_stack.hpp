#pragma once
#include <atomic>
#include <memory>
#include <utility>
#include <type_traits>


template<typename T>
class lock_free_stack {
private:
    struct node;

    struct counted_node_ptr {
        int external_count;
        node* ptr;
    };

    struct node {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count{0};
        counted_node_ptr next;

        node(T const& data)
            : data{std::make_shared<T>(data_)}
        {
        }

        template<typename... Args>
        node(std::in_place_t, Args&&... args)
            : data{std::make_shared<T>(std::forward<Args>(args)...)}
        {
        }
    };

    std::atomic<counted_node_ptr> head_;

    void increase_head_count(counted_node_ptr& old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        }
        while (!head_.compare_exchange_strong(old_counter, new_counter));
        
        old_counter.external_count = new_counter.external_count;
    }

public:
    ~lock_free_stack()
    {
        while(pop())
            ;
    }

    void push(T const& data)
    {
        counted_node_ptr new_node;
        new_node.external_count = 1;
        new_node.ptr = new node{data};

        new_node.ptr->next = head_.load();
        while (!head_.compare_exchange_weak(new_node.ptr->next, new_node))
            ;
    }

    std::shared_ptr<T> pop()
    {
        counted_node_ptr old_head = head_.load();
        for (;;)
        {
            increase_head_count(old_head);
            node* const ptr = old_head.ptr;
            if (!ptr)
            {
                return {nullptr};
            }
            if (head_.compare_exchange_strong(old_head, ptr->next))
            {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                int const count_increase = old_head.external_count - 2;
                if (ptr->internal_count.fetch_add(count_increase) == -count_increase)
                {
                    delete ptr;
                }
                return res;
            }
            else if (ptr->internal_count.fetch_sub(1) == 1)
            {
                delete ptr;
            }
        }
    }
};
