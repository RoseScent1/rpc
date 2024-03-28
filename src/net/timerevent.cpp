#include "timerevent.h"
#include "log.h"
#include "util.h"
namespace rocket {

TimerEvent::TimerEvent(int interval, bool is_repeated,
                       std::function<void()> callback)
    : interval_(interval), is_repeated_(is_repeated), callback_(callback) {
  ResetArriveTime();
}

void TimerEvent::SetCancel(bool cancel) { is_canceled_ = cancel; }

bool TimerEvent::IsCanceled() { return is_canceled_; }

bool TimerEvent::IsRepeated() { return is_repeated_; }

std::function<void()> TimerEvent::GetCallBack() { return callback_; }

int64_t TimerEvent::GetArriveTime() { return arrive_time_; }

void TimerEvent::ResetArriveTime() {
  arrive_time_ = getNowMs() + interval_;
  DEBUGLOG("successful create timer event, will excute at [%lld]",
           arrive_time_);
}

} // namespace rocket