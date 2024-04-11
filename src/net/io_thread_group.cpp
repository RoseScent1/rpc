#include "io_thread_group.h"
#include "io_thread.h"
#include "log.h"

namespace rocket {

IOThreadGroup::IOThreadGroup(int size) : size_(size), index_(-1) {
  iothread_groups_.resize(size_);
  for (auto &i : iothread_groups_) {
    i = new IOThread();
  }
}

IOThreadGroup::~IOThreadGroup() {

	// RPC_INFO_LOG("~IOThreadGroup");
  // for(auto i : iothread_groups_) {
  // 	i->Join();
  // }
}

void IOThreadGroup::Start() {
  for (auto i : iothread_groups_) {
    i->Start();
  }
}

void IOThreadGroup::Join() {
  for (auto i : iothread_groups_) {
    i->Join();
  }
}

IOThread *IOThreadGroup::GetIOThread() {
  index_ = (index_ + 1) % size_;
  return iothread_groups_[index_];
}

} // namespace rocket