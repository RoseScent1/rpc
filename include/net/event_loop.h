#pragma once
#include "fd_event.h"
#include "timer.h"
#include <functional>
#include <mutex>
#include <queue>
#include <sched.h>
#include <set>
#include <sys/epoll.h>
namespace rocket {
class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  void Loop();
  void WakeUp();
  void Stop();
  void AddEpollEvent(FdEvent *event);
  void DeleteEpollEvent(FdEvent *event);
  bool IsInLoopThread();
  void AddTask(std::function<void()> callback, bool wakeup = true);
	void AddTimerEvent(TimerEvent::s_ptr event);

	static EventLoop* GetCurrentEventLoop();
private:
  void InitWakeupEvent();
  void DealWakeUp();
  void InitTimer();
	
  bool is_stop_;

  pid_t thread_id_;

  int epoll_fd_;
  int wakeup_fd_;

  Timer *timer_;
  WakeUpFdEvent *wakeup_fd_event_{nullptr};
  std::mutex latch_;
  std::set<int> listen_fds_;
  std::queue<std::function<void()>> task_queue_;
	bool is_loop_{false};
};

} // namespace rocket