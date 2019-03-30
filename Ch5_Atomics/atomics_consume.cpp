#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <assert.h>

/* 
** Using release-consume synchronization ensures that the store to p
** only happens-before those expressions that are dependent on the value loaded
** from p. This means that the asserts on data members of the X structure
** are guaranteed not to trigger the assertion. However the assertion
** on the value of 'a' can trigger - this value isn't dependent on the value
** of 'p' so there's no synchronization in this case.
**
** Using the release-consume ordering causes the compiler to keep a 
** dependancy-chain. Sometimes it might a worthwile optimization to break it
** and allow the compiler to cache the values and reorder operations.
** In those scenarios std::kill_dependency() can be used. It copies the
** supplied argument to the return value and breaks the dependency-chain.
** e.g.
**
**  int global_data[] = {...}
**  std::atomic<int> index;
**  void f()
**  {
**       int i = index.load(std::memory_order_consume);
**       do_something_with(global_data[std::kill_dependency(i)]);
**  }
**
** This should be used with caution and as an optimization.
*/

struct X
{
    int i;
    std::string s;
};

std::atomic<X*> p;
std::atomic<int> a;

void create_x()
{
    X* x = new X;
    x->i = 42;
    x->s = "hello";
    a.store(99, std::memory_order_relaxed);
    p.store(x, std::memory_order_release);
}

void use_x()
{
    X* x;
    while( !(x=p.load(std::memory_order_consume)) ){
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
// --- all the expressions on which the value of *p depends on are guaranteed
// to be evaluated by this point
    assert(x->i == 42);
    assert(x->s == "hello");
// --- there's no guarantee for 'a'
    assert(a.load(std::memory_order_relaxed) == 99);
    std::cout << "x->i == " << x->i
              << "\nx->s == " << x->s
              << "\na == " << a.load(std::memory_order_relaxed)
              << std::endl;
}

int main()
{
    std::thread t1(create_x);
    std::thread t2(use_x);
    t1.join();
    t2.join();
    delete p;
    std::cout << "main: no asserts triggered" << std::endl;
}
