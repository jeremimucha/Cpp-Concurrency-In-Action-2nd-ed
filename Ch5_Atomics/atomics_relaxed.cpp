#include <iostream>
#include <atomic>
#include <thread>
#include <assert.h>


std::atomic<bool> x, y;
std::atomic<int> z;


void write_x_then_y()
{
    x.store(true, std::memory_order_relaxed);
    y.store(true, std::memory_order_relaxed);
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_relaxed))
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

// In the case of std::memory_order_relaxed even in such a simple case
// we have no guarantee that the following assert won't trigger.
// The only guarantee the relaxed memory order provides is that accesses to
// a single atomic variable on a single thread can't be reordered; this means
// that once a thread has read a particular variable any subsequent read can't
// return a value older than the one already seen. But there's no additional 
// synchronization - the operation order seen from the point of view of other
// threads can be different - there's no synchronizes-with relationship.
    assert(z.load() != 0);
    std::cout << "z.load() == " << z.load() << std::endl;
}