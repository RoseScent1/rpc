#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace rocket {

class TimerEvent {
public:
  using s_ptr = std::shared_ptr<TimerEvent>;
  TimerEvent(int interval, bool is_repeated, std::function<void()> callback);
  int64_t GetArriveTime();
  void SetCancel(bool cancel);
  bool IsCanceled();
  bool IsRepeated();
	std::function<void()> GetCallBack();
	void ResetArriveTime();
private:
  int64_t arrive_time_; // ms
  int64_t interval_;    // ms
  std::function<void()> callback_;
  bool is_repeated_; // 是否重复
  bool is_canceled_; //是否中止
};
} // namespace rocket