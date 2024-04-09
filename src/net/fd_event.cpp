
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iterator>
#include <mutex>
#include <queue>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fd_event.h"
#include "log.h"
namespace rocket {
// FdEvent

FdEvent::FdEvent(int fd) : fd_(fd) {
  memset(&listen_event_, 0, sizeof(listen_event_));
}
FdEvent::FdEvent() { memset(&listen_event_, 0, sizeof(listen_event_)); }
FdEvent::~FdEvent() {
  // INFOLOG("~FdEvent");
}
std::function<void()> FdEvent::Handler(TriggerEvent event_type) {
  if (event_type == TriggerEvent::IN_EVENT) {
    return read_callback_;
  } else if (event_type == TriggerEvent::OUT_EVENT) {
    return write_callback_;
  }
  return err_callback_;
}

void FdEvent::Listen(TriggerEvent event_type, std::function<void()> callback) {
  if (event_type == TriggerEvent::IN_EVENT) {
    listen_event_.events |= EPOLLIN;
    read_callback_ = callback;
  } else if (event_type == TriggerEvent::OUT_EVENT) {
    listen_event_.events |= EPOLLOUT;
    write_callback_ = callback;
  } else {
    err_callback_ = callback;
  }
  listen_event_.data.ptr = this;
}

void FdEvent::Cancel(TriggerEvent event_type) {
  if (event_type == TriggerEvent::IN_EVENT) {
    listen_event_.events &= ~EPOLLIN;
  } else {
    listen_event_.events &= ~EPOLLOUT;
  }
}

// WakeUpFdEvent
WakeUpFdEvent::WakeUpFdEvent(int fd) : FdEvent(fd) {}

WakeUpFdEvent::~WakeUpFdEvent(){
    // INFOLOG("~WakeUpFdEvent");
};

void WakeUpFdEvent::WakeUp() {
  char buff[8] = "a";
  int rt = write(fd_, buff, 8);
  if (rt != 8) {
    ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", fd_);
  }
}

void FdEvent::SetNonBlock() {
  int flag = fcntl(fd_, F_GETFL, 0);
  if (flag & O_NONBLOCK) {
    return;
  }
  fcntl(fd_, F_SETFL, flag & O_NONBLOCK);
}
} // namespace rocket