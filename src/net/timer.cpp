#include "timer.h"
#include "event_loop.h"
#include "log.h"
#include "timer_event.h"
#include "util.h"
#include <asm-generic/errno-base.h>
#include <bits/types/struct_itimerspec.h>
#include <bits/types/struct_timespec.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/timerfd.h>
#include <unistd.h>
namespace rocket {
Timer::Timer() {

  fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  DEBUGLOG("timer fd = %d", fd_);
  //把fd的可读事件放到fd上监听
  Listen(FdEvent::IN_EVENT, std::bind(&Timer::OnTimer, this));
}

Timer::~Timer() {}

void Timer::ResetArriveTime() {
  std::unique_lock<std::mutex> lock(latch_);
  if (timer_events_.empty()) {
    return;
  }
  int64_t now_time = getNowMs();
  auto it = timer_events_.begin();
  int64_t interval = 0;
  if (it->second->GetArriveTime() > now_time) {
    interval = it->second->GetArriveTime() - now_time;
  } else {
    interval = 100;
  }
  timespec ts;
  memset(&ts, 0, sizeof(ts));
  ts.tv_sec = interval / 1000;
  ts.tv_nsec = interval % 1000 * 1e6;
  itimerspec value;
  memset(&value, 0, sizeof(value));
  value.it_value = ts;
  int rt = timerfd_settime(fd_, 0, &value, NULL);
  if (rt != 0) {
    ERRORLOG("timerfd_settime error, errno = %d, %s", errno,
             std::strerror(errno));
  }
  DEBUGLOG("fd = %d, timer reset to %lld", fd_, now_time + interval);
}

void Timer::AddTimerEvent(TimerEvent::s_ptr event) {
  bool is_reset_timerfd = false;
  std::unique_lock<std::mutex> lock(latch_);
  if (timer_events_.empty() ||
      timer_events_.begin()->second->GetArriveTime() > event->GetArriveTime()) {
    is_reset_timerfd = true;
  }
  timer_events_.emplace(event->GetArriveTime(), event);
  lock.unlock();
  if (is_reset_timerfd) {
    ResetArriveTime();
  }
}

void Timer::DeleteTimerEvent(TimerEvent::s_ptr event) {
  event->SetCancel(true);
  std::unique_lock<std::mutex> lock(latch_);
  auto begin = timer_events_.lower_bound(event->GetArriveTime());
  auto end = timer_events_.upper_bound(event->GetArriveTime());
  while (begin != end) {
    if (begin->second == event) {
      timer_events_.erase(begin);
      break;
    }
    ++begin;
  }
  DEBUGLOG("successful delete Timer event fd = %lld", event->GetArriveTime());
}

void Timer::OnTimer() {
  // 处理缓冲区
  char buff[8];
  while (read(fd_, buff, 8) != -1 && errno != EAGAIN)
    ;

  // 执行定时任务
  int64_t now = getNowMs();
  std::vector<TimerEvent::s_ptr> tmp;
  std::unique_lock<std::mutex> lock(latch_);
  std::vector<std::function<void()>> task;
  auto it = timer_events_.begin();
  for (; it != timer_events_.end(); ++it) {
    if (it->first <= now) {
      if (!it->second->IsCanceled()) {
        tmp.emplace_back(it->second);
        task.emplace_back(it->second->GetCallBack());
      }
    } else {
      break;
    }
  }
  timer_events_.erase(timer_events_.begin(), it);
  lock.unlock();

  // 把需要重复的任务再添加进去
  for (auto &i : tmp) {
    if (i->IsRepeated()) {
      i->ResetArriveTime();
      AddTimerEvent(i);
    }
  }
  ResetArriveTime();
  //调用回调函数
  for (auto &i : task) {
    i();
  }
}

} // namespace rocket