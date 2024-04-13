#pragma once
#include "event_loop.h"
#include <semaphore.h>
#include <semaphore>
#include <sys/types.h>
#include <thread>
namespace rocket {

class IOThread {
public:
  IOThread();
  ~IOThread();
  EventLoop *GetEventloop();

	void Start();
	void Join();
  static void Main(IOThread *thread);

private:
  int32_t thread_id_;
  std::thread thread_;
  EventLoop *event_loop_;
  sem_t sem_init_;
	sem_t sem_start_;
};
} // namespace rocket