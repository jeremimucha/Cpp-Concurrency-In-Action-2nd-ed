#include <iostream>
#include <atomic>
#include <thread>
#include <assert.h>
/* 
** Acquire-release operations can impose ordering on relaxed operations.
*/


std::atomic<bool> x, y;
std::atomic<int> z;


void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);
    y.store(true, std::memory_order_release);
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_acquire))
        ;
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

// In this case we are guaranteed that this assertion won't be triggered.
// Using a memory_order_release store operation paired with a memory_order_acquire
// load operation (on the same atomic) ensures that effects of all the operations
// (on the same thread) before the .store(true, std::memory_order_release)
// are seen after the load(std::memory_order_acquire) (on the same atomic).
    assert(z.load() != 0);
    std::cout << "z.load() == " << z.load() << std::endl;
}