#pragma once
#include <atomic>
#include <memory>
#include <utility>
#include <type_traits>


template<typename T>
class lock_free_stack {
private:
    struct node {
        std::shared_ptr<T> data;
        node* next;

        node(T const& data_)
            : data{std::make_shared<T>(data_)}
        {
        }

        node(T&& data_)
            : data{std::make_shared<T>(std::move(data_))}
        {
        }

        template<typename... Args>
        node(std::in_place_t, Args&&... args)
            : data{std::make_shared<T>(std::forward<Args>(args)...)}
        {
        }
    };

    std::atomic<node*> head_;
    std::atomic<unsigned> threads_in_pop_{0};
    std::atomic<node*> to_be_deleted_;


    static void delete_nodes(node* nodes)
    {
        while (nodes)
        {
            // node* next = nodes->next;
            // delete nodes;
            // nodes = next;
            node* node = std::exchange(nodes, nodes->next);
            delete node;
        }
    }

    void try_reclaim(node* old_head)
    {
        if (threads_in_pop_ == 1)
        {
            // take ownership of to-be-deleted nodes
            node* nodes_to_delete = to_be_deleted_.exchange(nullptr);
            // check if we're still the only thread in pop
            if (!--threads_in_pop_)
            {
                delete_nodes(nodes_to_delete);
            }
            // otherwise put the nodes to delete back in the delete queue
            else if (nodes_to_delete)
            {
                chain_pending_nodes(nodes_to_delete);
            }
            // If we got here it's always safe to delete the node
            // we poped originally.
            delete old_head;
        }
        else
        {
            chain_pending_node(old_head);
            --threads_in_pop_;
        }
    }

    void chain_pending_nodes(nodes* nodes)
    {
        // We need the first and last node
        // of the to-delete-linked-list, if we want to
        // add it to the queue
        node* last = nodes;
        while (node* const next = last->next)
        {
            last = next;
        }
        chain_pending_nodes(nodes, last);
    }

    void chain_pending_nodes(node* first, node* last)
    {
        // push the pending_nodes to the front of the to-be-deleted queue
        last->next = to_be_deleted_;
        // make sure we see the changes any other threads may have made.
        while (!to_be_deleted_.compare_exchange_weak(last->next, first))
            ;
    }

    // convenience function for a single-node case
    void chain_pending_node(node* n)
    {
        chain_pending_nodes(n, n);
    }

public:
    void push(T const& data)
    {
        // allocation here can throw - we haven't mutated anything yet,
        // so we're safe.
        node* const new_node = new node(data);
        new_node->next = head_.load();
        // We update the head_ to point to the new_node, but only if
        // head_ hasn't been modified by another thread since we've set
        // new_node-next to it; compare_exchange_weak will update
        // the new_node->next if it no longer points to head_,
        // otherwise it will set head_ to new_node.
        while (!head_.compare_exchange_weak(new_node->next, new_node))
            ;
    }

    std::shared_ptr<T> pop()
    {
        node* old_head = head_.load();  // #1
        while (old_head &&
               !head_.compare_exchange_weak(old_head, old_head->next))  // #2
            ;
        return old_head ? old_head->data : {nullptr};
        // !!! Note that we leak the node here! This is because if we deleted
        // the node, but another thread just read it #1 and hasn't reached #2
        // before we deleted the node, the other thread would then dereference
        // a nullptr in step #2.
        // TODO: how to handle this?
        // - for a single consumer (only one thread calling pop()) this is
        //   trivial - we could just delete the node here.
        // - multiple-consumer implementations need to keep track of the
        //   popped nodes - by pushing them to a `to-be-deleted` queue.
        //   The nodes on the delete-queue can only be deleted when
        //   there's no threads calling pop - this needs to be tracked
        //   with an atomic counter.
    }

    std::shared_ptr<T> single_consumer_pop()
    {
        node* old_head = head_.load();  // #1
        while (old_head &&
               !head_.compare_exchange_weak(old_head, old_head->next))  // #2
            ;

        auto val = old_head ? std::move(old_head->data) : {nullptr};
        delete old_head;
        return val;
    }

    std::shared_ptr<T> multi_consumer_pop()
    {
        ++threads_in_pop_;
        node* old_head = head_.load();
        while (old_head &&
               !head_.compare_exchange_weak(old_head, old_head->next))
            ;
        std::shared_ptr<T> res;
        if (old_head)
        {
            res.swap(old_head->data);
        }
        try_reclaim(old_head);
        return res;
    }

    /// Invalid implementation attempt - impossible to guarantee
    /// exception safety when data is held by-value and returned
    /// via the `result` out param.
    // void pop(T& result)
    // {
    //     node* old_head = head_.load();  // #1
    //     while (!head_.compare_exchange_weak(old_head, old_head->next))  // #2
    //         ;
    //     result = old_head->data;
    //     // !!! Note that we leak the node here! This is because if we deleted
    //     // the node, but another thread just read it #1 and hasn't reached #2
    //     // before we deleted the node, the other thread would then dereference
    //     // a nullptr in step #2.
    //     // TODO: how to handle this?
    // }
};
