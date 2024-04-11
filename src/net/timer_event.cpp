#include "timer_event.h"
#include "log.h"
#include "util.h"
namespace rocket {

TimerEvent::TimerEvent(int interval, bool is_repeated,
                       std::function<void()> callback)
    : interval_(interval),  callback_(callback),is_repeated_(is_repeated) {
  ResetArriveTime();
}

void TimerEvent::SetCancel(bool cancel) { is_canceled_ = cancel; }

bool TimerEvent::IsCanceled() { return is_canceled_; }

bool TimerEvent::IsRepeated() { return is_repeated_; }

std::function<void()> TimerEvent::GetCallBack() { return callback_; }

int64_t TimerEvent::GetArriveTime() { return arrive_time_; }

void TimerEvent::ResetArriveTime() {
  arrive_time_ = getNowMs() + interval_;
  RPC_DEBUG_LOG("successful Reset timer event, will excute at [%lld]",
           arrive_time_);
}

} // namespace rocket