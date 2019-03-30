#include <iostream>
#include <thread>
#include <atomic>
#include <assert.h>


std::atomic<bool> x, y;
std::atomic<int> z;


void write_x()
{
// std::memory_order_seq_cst is the default - it enforces sequentially consistent
// memory order across all threads. It could (will) cause some overhead
// in desctop processors - minor, but in some cases large.
    x.store(true, std::memory_order_seq_cst);
}

void write_y()
{
    y.store(true, std::memory_order_seq_cst);
}

void read_x_then_y()
{
    while(!x.load(std::memory_order_seq_cst))
        ;
    if(y.load(std::memory_order_seq_cst))
        ++z;
}

void read_y_then_x()
{
    while(!y.load(std::memory_order_seq_cst))
        ;
    if(x.load(std::memory_order_seq_cst))
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

// The read of z is guaranteed to be greater than 0.
// Both read_x_then_y() and read_y_then_x() loop until one of the variables
// is == true, sequantially consistent memory order guarantees that all threads
// will se the same state of atomic variables - which means that after
// x == true on thread c it will also == true on thread d; we're also guaranteed
// that after y == true on thread d it will == true on thread c therefore
// at least one of the if() statements will evaluate to true by the time
// the threads c and d reach them.
// z might equal 2 or 1, but not 0;
    assert(z.load() != 0);
    std::cout << "z.load() == " << z.load() << std::endl;
}