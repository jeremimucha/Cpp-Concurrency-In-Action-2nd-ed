#include <iostream>
#include <atomic>
#include <thread>
#include <assert.h>

/* 
** This program is effectively the same as atomics_fences.cpp, the assert
** is also guaranteed not to fire in this example, even though 'x' is
** a non-atomic variable. It's still synchronized by the fence.
** Non-atomics can also be synchronized by .stores() and .loads()
** using the std::memory_order_release / std::memory_order_acquire
** memory orderings. Most of the atomic operations using
** std::memory_order_relaxed can be replaces with non-atomic types.
*/

bool x;
std::atomic<bool> y;
std::atomic<int> z;


void write_x_then_y()
{
    x = true;
    std::atomic_thread_fence(std::memory_order_release);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_relaxed))
        ;
    std::atomic_thread_fence(std::memory_order_acquire);
    if(x){
        ++z;
    }
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
