#include <iostream>
#include <atomic>
#include <thread>
#include <assert.h>

/* 
** The effect of the fences used in this program is effectively the same
** as if the memory order used on the 'y' atomic was std::memory_order_release
** for the store, and std::memory_order_acquire for the load.
** All the operations that happen before the fence are synchronized
** at the point the fence is placed.
*/


std::atomic<bool> x, y;
std::atomic<int> z;


void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_relaxed))
        ;
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x.load(std::memory_order_relaxed))
        ++z;
}


int main()
{
    x = false;
    y = false;
    z = 0;
    std::thread a(write_x_then_y);
    std::thread b(read_y_then_x);
    a.join();
    b.join();
// This assert is guaranteed not to be triggered
    assert(z.load() != 0);
    std::cout << "z.load() == " << z.load() << std::endl;
}
