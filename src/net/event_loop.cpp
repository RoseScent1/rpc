#include "event_loop.h"
#include "log.h"
#include "timer.h"
#include "timer_event.h"
#include "util.h"
#include <algorithm>
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
namespace rocket {

static int epoll_max_timeout = 10000;
static int epoll_max_events = 10;
static thread_local std::unique_ptr<EventLoop> current_eventloop = nullptr;
// Eventloop
EventLoop::EventLoop() : is_stop_(false) {
  // 每个线程只能创建一个eventloop
  if (current_eventloop != nullptr) {
    ERRORLOG(
        "failed to crreate event loop, this thread has created event loop");
    exit(0);
  }
  thread_id_ = getThreadid();

  // 创建此线程的epoll文件描述符
  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    ERRORLOG(
        "failed to crreate event loop, epoll_fd  create error, error info[%d]",
        errno);
    exit(0);
  }
  InitWakeupEvent();
  InitTimer();
  AddEpollEvent(wakeup_fd_event_);
  INFOLOG("successful create wakeup event loop in thread %d", thread_id_);
	// current_eventloop.reset(this);
}

EventLoop::~EventLoop() {
  close(epoll_fd_);
  close(wakeup_fd_);
  if (wakeup_fd_event_) {
    delete wakeup_fd_event_;
    wakeup_fd_event_ = nullptr;
  }
  if (timer_ != nullptr) {
    delete timer_;
    timer_ = nullptr;
  }
  // INFOLOG("~EventLoop");
}

void EventLoop::InitTimer() {
  timer_ = new Timer();
  AddEpollEvent(timer_);
}

void EventLoop::InitWakeupEvent() {
  wakeup_fd_ = eventfd(0, EFD_NONBLOCK);
  if (wakeup_fd_ < 0) {
    ERRORLOG(
        "failed to create event loop, wakeup_fd create error, error info[%d]",
        errno);
    exit(0);
  }
  wakeup_fd_event_ = new WakeUpFdEvent(wakeup_fd_);
  wakeup_fd_event_->Listen(FdEvent::IN_EVENT, [this]() {
    char buff[8];
    while (read(wakeup_fd_, buff, 8) != -1 && errno != EAGAIN)
      ;
    DEBUGLOG("read full bytes from wakeup fd[%d]", wakeup_fd_);
  });
  DEBUGLOG("create wake up fd, fd = %d", wakeup_fd_);
}

void EventLoop::AddTimerEvent(TimerEvent::s_ptr event) {
  timer_->AddTimerEvent(event);
}

void EventLoop::Loop() {
  while (!is_stop_) {
    std::unique_lock<std::mutex> lock(latch_);
    std::queue<std::function<void()>> task_queue;
    task_queue.swap(task_queue_);
    lock.unlock();
    while (!task_queue.empty()) {
      task_queue.front()();
      task_queue.pop();
    }
    if (is_loop_) {
      return;
    }
    int timeout = epoll_max_timeout;
    epoll_event result_events[epoll_max_events];
    for (int i = 0; i < epoll_max_events; ++i) {
      result_events[i].events = 0;
    }
    DEBUGLOG("begin to wait");
    int rt = epoll_wait(epoll_fd_, result_events, epoll_max_events, timeout);
    DEBUGLOG("end  epoll_wait rt= %d", rt);
    if (rt < 0) {
      ERRORLOG("epoll_wait error, error info[%s]", strerror(errno));
    } else {
      for (int i = 0; i < rt; ++i) {
        auto trigger_event = result_events[i];
        auto fd_event = static_cast<FdEvent *>(trigger_event.data.ptr);
        if (fd_event == nullptr) {
          continue;
        }
        if (trigger_event.events & EPOLLIN) {
          DEBUGLOG("fd[%d] trigger EPOLLIN", fd_event->GetFd())
          AddTask(fd_event->Handler(FdEvent::IN_EVENT));
        }
        if (trigger_event.events & EPOLLOUT) {
          DEBUGLOG("fd[%d] trigger EPOLLOUT", fd_event->GetFd())
          AddTask(fd_event->Handler(FdEvent::OUT_EVENT));
        }
      }
    }
  }
  is_loop_ = true;
}

void EventLoop::WakeUp() {
  INFOLOG("wake up fd=%d", wakeup_fd_);
  wakeup_fd_event_->WakeUp();
}

void EventLoop::Stop() { is_stop_ = true; }

void EventLoop::AddEpollEvent(FdEvent *event) {
  if (IsInLoopThread()) {
    int op = EPOLL_CTL_ADD;
    if (listen_fds_.find(event->GetFd()) != listen_fds_.end()) {
      op = EPOLL_CTL_MOD;
    }
    epoll_event tmp = event->GetEpollEvent();
    INFOLOG("add event events[%d],fd = %d", (int)tmp.events, event->GetFd());
    if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
      ERRORLOG("failed epoll_ctl when add fd %d, error info[%d] =%s",
               event->GetFd(), errno, strerror(errno));
    }
    listen_fds_.insert(event->GetFd());
    DEBUGLOG("add event success, fd[%d]", event->GetFd());
  } else {
    auto cb = [this, event]() {
      int op = EPOLL_CTL_ADD;
      if (listen_fds_.find(event->GetFd()) != listen_fds_.end()) {
        op = EPOLL_CTL_MOD;
      }
      epoll_event tmp = event->GetEpollEvent();
      INFOLOG("add event events[%d],fd = %d", (int)tmp.events, event->GetFd());
      if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
        ERRORLOG("failed epoll_ctl when add fd %d, error info[%d] = ",
                 event->GetFd(), errno, strerror(errno));
      }
      listen_fds_.insert(event->GetFd());
      DEBUGLOG("add event success, fd[%d]", event->GetFd());
    };
    AddTask(cb);
  };
}

void EventLoop::DeleteEpollEvent(FdEvent *event) {
  if (IsInLoopThread()) {
    if (listen_fds_.find(event->GetFd()) == listen_fds_.end()) {
      return;
    }
    int op = EPOLL_CTL_DEL;
    epoll_event tmp = event->GetEpollEvent();

    if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
      ERRORLOG("failed epoll_ctl when delete fd %d, error info[%d] = ",
               event->GetFd(), errno, strerror(errno));
    }
    listen_fds_.erase(event->GetFd());
    DEBUGLOG("delete event success, fd[%d]", event->GetFd());
  } else {
    auto cb = [this, event] {
      if (listen_fds_.find(event->GetFd()) == listen_fds_.end()) {
        return;
      }
      int op = EPOLL_CTL_DEL;
      epoll_event tmp = event->GetEpollEvent();
      if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
        ERRORLOG("failed epoll_ctl when delete fd %d, error info[%d] = ",
                 event->GetFd(), errno, strerror(errno));
      }
      listen_fds_.erase(event->GetFd());
      DEBUGLOG("delete event success, fd[%d]", event->GetFd());
    };
    AddTask(cb, true);
  }
}

void EventLoop::DealWakeUp() {}
bool EventLoop::IsInLoopThread() { return getThreadid() == thread_id_; }
void EventLoop::AddTask(std::function<void()> callback, bool wakeup) {
  std::unique_lock<std::mutex> lock(latch_);
  task_queue_.push(callback);
  if (wakeup) {
    WakeUp();
  }
}

EventLoop *EventLoop::GetCurrentEventLoop() {
  if (current_eventloop == nullptr)  {
		current_eventloop.reset(new EventLoop());
	}
	return current_eventloop.get();
}
} // namespace rocket
