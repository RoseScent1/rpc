#pragma once
#include "fd_event.h"
#include <memory>
#include <mutex>

namespace rocket {

class FdEventGroup {
public:
  FdEventGroup(int size);
  ~FdEventGroup();
	std::shared_ptr<FdEvent> GetFdEvent(int fd);

	static FdEventGroup* GetFdEventGRoup();
private:
  int size_{0};
	std::mutex latch_;
  std::vector<std::shared_ptr<FdEvent>> fd_event_group_;
};
} // namespace rocket