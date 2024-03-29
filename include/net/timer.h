#pragma once
#include "fd_event.h"
#include "timer_event.h"
#include <cstdint>
#include <map>
#include <mutex>
namespace rocket {
class Timer : public FdEvent {
public:
  Timer();
  ~Timer();

  void AddTimerEvent(TimerEvent::s_ptr event);
  void DeleteTimerEvent(TimerEvent::s_ptr event);
  void OnTimer(); // 到达时间戳,触发指定事件

private:
  // 重置触发时间
  void ResetArriveTime();

  std::multimap<int64_t, TimerEvent::s_ptr> timer_events_;
  std::mutex latch_;
};
} // namespace rocket