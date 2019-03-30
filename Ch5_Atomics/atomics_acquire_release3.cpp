#include <iostream>
#include <thread>
#include <atomic>
#include <assert.h>


std::atomic<int> data[5];
std::atomic<bool> sync1(false), sync2(false);
// --- alternatively - use one atomic instead two for synchronization
// std::atomic<int> sync(0);

void thread1()
{
    data[0].store(42, std::memory_order_relaxed);
    data[1].store(97, std::memory_order_relaxed);
    data[2].store(17, std::memory_order_relaxed);
    data[3].store(-141, std::memory_order_relaxed);
    data[4].store(2003, std::memory_order_relaxed);
// using a single acquire-release synchronized atomic is enough to synchronize
// all the atomics touched up to this point across threads
// using acquire-release sychronization on the same variables
    sync1.store(true, std::memory_order_release);
// --- alternative with one variable
    // sync.store(1, std::memory_order_release);
}

void thread2()
{
    while(!sync1.load(std::memory_order_acquire))
        ;
    sync2.store(true, std::memory_order_release);
// --- alternative with one variable
// A `read-modify-write` operation with std::memory_order_acq_rel semantics can synchronize
// with both a prior `store` operation with std::memory_oredr_release semantics,
// AND with a subsequent `load` operation with std::memory_order acquire semantics,
    // int expected = 1;
    // while(!sync.compare_exchange_strong(expected, 2, std::memory_order_acq_rel)){
    //     expected = 1;
    // }
}

void thread3()
{
    while(!sync2.load(std::memory_order_acquire))
        ;
// --- alternative with one variable
    // while( sync.load(std::memory_order_acquire) < 2 )
    //     ;
    assert( data[0].load(std::memory_order_relaxed) == 42 );
    assert( data[1].load(std::memory_order_relaxed) == 97 );
    assert( data[2].load(std::memory_order_relaxed) == 17 );
    assert( data[3].load(std::memory_order_relaxed) == -141 );
    assert( data[4].load(std::memory_order_relaxed) == 2003 );

    std::cout << "data[0] = " 
              << data[0].load(std::memory_order_relaxed) << std::endl;
    std::cout << "data[1] = " 
              << data[1].load(std::memory_order_relaxed) << std::endl;
    std::cout << "data[2] = " 
              << data[2].load(std::memory_order_relaxed) << std::endl;
    std::cout << "data[3] = " 
              << data[3].load(std::memory_order_relaxed) << std::endl;
    std::cout << "data[4] = " 
              << data[4].load(std::memory_order_relaxed) << std::endl;
}


int main()
{
    std::thread t1(thread1);
    std::thread t2(thread2);
    std::thread t3(thread3);

    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "main - no asserts triggered " << std::endl;
}
