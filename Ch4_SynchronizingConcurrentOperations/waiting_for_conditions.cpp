#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
/*
 * Threads need to perform operations in response to other operations being
 * completed. C++ provides condition variables for this purpose.
 * They are used in tandem with mutexes to wake the thread up once a given
 * condition is satisfied.
 * Condition variables should be used together with std::unique_lock in calls
 * to condition_variable::wait(), because wait() needs to unlock() and lock()
 * the mutex again during it's operation.
 * See also threadsafe_queue.hpp for an implementation of a threadsafe queue
 * using std::mutex and std::condition_variable
 */

std::mutex mtx;
std::queue<int> data_queue;
std::condition_variable data_cond;

bool more_data_to_prepare() {
  static int i{0};
  return i++ < 11;
}

int prepare_data() {
  static std::default_random_engine re{static_cast<unsigned>(
      std::chrono::system_clock::now().time_since_epoch().count())};
  static std::uniform_int_distribution<> ud(300, 800);
  static int i{0};
  std::this_thread::sleep_for(std::chrono::milliseconds{ud(re)});
  return i++;
}

void process(int i) {
  std::cerr << "thread[" << std::this_thread::get_id() << "] data = " << i
            << "\n";
}

bool is_last_chunk(int data) {
  return !(data < 10);
}

void data_preparation_thread() {
  /*
   * Prepare data and notify a thread that data it is waiting for might be
   * ready. It causes a thread to wake up, lock the mutex, check it's given
   * condition and proceed accordingly to the result
   * - (if condition is true) keep the mutex locked and proceed further,
   * - (if condition is false) unlock the mutex and keep waiting until notified
   */
  while (more_data_to_prepare()) {
    const int data = prepare_data();
    std::lock_guard<std::mutex> lk{mtx};
    std::cerr << "thread[" << std::this_thread::get_id()
              << "] pushing data = " << data << "\n";
    data_queue.push(data);
    data_cond.notify_one();
  }
}

void data_processing_thread() {
  while (true) {
    std::unique_lock<std::mutex> lk{mtx};
    data_cond.wait(lk, [] {
      return !data_queue.empty();
    });  // wait using condition_variable
    int data = data_queue.front();
    data_queue.pop();
    /* std::unique_lock is also usefull here because it allows us to unlock the
     ** mutex before processing the data - the called function might lock other
     ** mutexes or might be time consuming - it's a bad idea to hold a mutex
     *longer
     ** the necessary in the latter case. */
    lk.unlock();
    process(data);
    if (is_last_chunk(data)) {
      break;
    }
  }
}

int main() {
  std::thread prep_thread{data_preparation_thread};
  std::thread processing_thread{data_processing_thread};

  processing_thread.join();
  prep_thread.join();
}
