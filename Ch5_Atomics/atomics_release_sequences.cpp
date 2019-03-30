#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

/**
 * If a `store` operation is std::memory_order_release (or seq_cst) and a `load` operation is
 * std::memory_order_acquire (or seq_cst) and each operation in the chain loads the value written
 * by previous operation (implying read-modify-write operations in between the initial `store`
 * and synchronizing `load`) then the chain is a `release-sequence`, and the initial `store
 * synchronizes-with the `load`.
 */

std::vector<int> queue_data;
std::atomic<int> count;


void populate_queue()
{
    std::cout << "populate_queue on thread[" << std::this_thread::get_id()
        << "]" << std::endl;
    const unsigned number_of_items = 20;
    queue_data.clear();
    for(unsigned i=0; i<number_of_items; ++i){
        queue_data.push_back(i);
    }

    count.store(number_of_items, std::memory_order_release);
}

void wait_for_more_items()
{
    std::this_thread::sleep_for(std::chrono::microseconds(11));
    std::this_thread::yield();
}

void process(int item)
{
    std::cout << "thread[" << std::this_thread::get_id()
        << "] process: " << item << std::endl;
}

void consume_queue_items()
{
    std::cout << "consume_queue_items on thread[" << std::this_thread::get_id()
        << "]" << std::endl;
    while(true){
        int item_index;
        if( (item_index = count.fetch_sub(1, std::memory_order_acquire)) <= 0 ){
            wait_for_more_items();
            continue;
        }

        process(queue_data[item_index-1]);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);
    a.join();
    b.join();
    c.join();
}
