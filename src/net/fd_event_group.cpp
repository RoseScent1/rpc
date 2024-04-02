#include "fd_event.h"
#include "fd_event_group.h"
#include <memory>
#include <mutex>

namespace rocket {
static FdEventGroup *g_fd_event_group = nullptr;

FdEventGroup::FdEventGroup(int size) : size_(size) {
  for (int i = 0; i < size; ++i) {
    fd_event_group_.emplace_back(new FdEvent(i));
  }
}
FdEventGroup::~FdEventGroup() {}

std::shared_ptr<FdEvent> FdEventGroup::GetFdEvent(int fd) {
  std::unique_lock<std::mutex> lock(latch_);
  if (fd < size_) {
    return fd_event_group_[fd];
  }
  int new_size = fd * 1.5;
  for (int i = size_; i < new_size; ++i) {
    fd_event_group_.emplace_back(new FdEvent(i));
  }
  size_ = new_size;
	return fd_event_group_[fd];
}

FdEventGroup *FdEventGroup::GetFdEventGRoup() {
  return g_fd_event_group ? g_fd_event_group
                          : g_fd_event_group = new FdEventGroup(128);
}
} // namespace rocket