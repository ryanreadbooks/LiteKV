#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "../src/net/net.h"

void Func1() {
  static int i1 = 0;
  std::cout << "INFO: Func1 called at " << GetCurrentMs() << " cnt = " << ++i1 << std::endl;
}

void Func2() {
  static int i1 = 0;
  std::cout << "ERROR: Func2 called at " << GetCurrentMs() << " cnt = " << ++i1 << std::endl;
}

void Func3() {
  static int i1 = 0;
  std::cout << "WARN: Func3 called at " << GetCurrentMs() << " cnt = " << ++i1 << std::endl;
}

void Func4() {
  static int i1 = 0;
  std::cout << "FATAL: Func4 called at " << GetCurrentMs() << " cnt = " << ++i1 << std::endl;
}

int main() {
  EventLoop loop;
  auto now = GetCurrentMs();
  std::cout << "Now   is " << now << std::endl;
  /* execute after 1000ms, will only be executed once */
  std::cout << loop.tev_holder->CreateTimeEvent(1000, std::function<void()>(Func1)) << std::endl;
  /* execute every 5000 ms for 3 times */
  std::cout << loop.AddTimeEvent(2000, std::function<void()>(Func2), 3) << std::endl;
  /* this one runs forever */
  std::cout << loop.AddTimeEvent(1000, Func3, FIRE_FOREVER)->id << std::endl;
  //  std::cout << loop.AddTimeEvent(10000, Func4, 2)->id << std::endl;

  std::mutex mtx;
  std::condition_variable cond;
  // this thread is used to exit the event loop
  std::thread cleanup_worker([&]() {
    std::unique_lock<std::mutex> lck(mtx);
    cond.wait_for(lck, std::chrono::seconds(5), [] { return false; });
    loop.stopped.store(true);
  });

  loop.Loop();
  cleanup_worker.join();
  return 0;
}