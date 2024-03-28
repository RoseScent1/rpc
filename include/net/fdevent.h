#pragma  once
#include <functional>
#include <mutex>
#include <queue>
#include <sched.h>
#include <set>
#include <sys/epoll.h>
namespace rocket {

class FdEvent {
public:
  enum TriggerEvent { IN_EVENT = EPOLLIN, OUT_EVENT = EPOLLOUT };
  FdEvent(int fd);
  FdEvent();
  ~FdEvent();
  std::function<void()> Handler(TriggerEvent event_type);
  void Listen(TriggerEvent event_type, std::function<void()> callback);
  int GetFd() const { return fd_; }
  epoll_event GetEpollEvent() { return listen_event_; }

protected:
  int fd_;
  epoll_event listen_event_;
  std::function<void()> read_callback_;
  std::function<void()> write_callback_;
};

class WakeUpFdEvent : public FdEvent {
public:
  WakeUpFdEvent(int fd);
  ~WakeUpFdEvent();
  void WakeUp();
};
} // namespace rocket