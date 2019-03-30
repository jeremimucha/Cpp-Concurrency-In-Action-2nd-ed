#include <iostream>
#include <thread>
#include <atomic>
#include <assert.h>
/* acquire-release ordering treats atomic loads as acquire operations
** and atomics stores as release operations and atomic read-modify-write
** as either acquire, release or both - acq_rel. Synchronization is pair-wise
** between the thread that does the release and the thread that does the
** acquire. A release operation synchronizes-with an acquire operation that
** reads the value written; but different threads can still see different
** orderings. */

std::atomic<bool> x, y;
std::atomic<int> z;


void write_x()
{
    x.store(true, std::memory_order_release);
}

void write_y()
{
    y.store(true, std::memory_order_release);
}

void read_x_then_y()
{
    while(!x.load(std::memory_order_acquire))
        ;
    if(y.load(std::memory_order_acquire))
        ++z;
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_acquire))
        ;
    if(x.load(std::memory_order_acquire))
        ++z;
}


int main()
{
    x = false;
    y = false;
    z = 0;

    std::thread a(write_x);
    std::thread b(write_y);
    std::thread c(read_x_then_y);
    std::thread d(read_y_then_x);

    a.join();
    b.join();
    c.join();
    d.join();

// In this case the assert can still be triggered, because both .store()
// operations happen in different threads. In order to benefit from
// acquire-release ordering we need to do the store operations in a single
// thread - to ensure an ordering -- see atomics_acquire_release2.cpp
    assert(z.load() != 0);
    std::cout << "z.load() == " << z.load() << std::endl;
}