#include "event_loop.h"
#include "fd_event.h"
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
static thread_local std::unique_ptr<EventLoop> current_event_loop = nullptr;
// Event_loop
EventLoop::EventLoop() : is_stop_(false) {
  // 每个线程只能创建一个event_loop
  if (current_event_loop != nullptr) {
    RPC_ERROR_LOG(
        "failed to create event loop, this thread has created event loop");
    exit(1);
  }
  thread_id_ = getThreadid();

  // 创建此线程的epoll文件描述符
  epoll_fd_ = epoll_create(100000);
  APP_DEBUG_LOG("epoll_fd %d created", epoll_fd_);

  RPC_DEBUG_LOG("```epoll_fd = [%d]```", epoll_fd_);
  if (epoll_fd_ == -1) {
    RPC_ERROR_LOG(
        "failed to create event loop, epoll_fd  create error, error info[%d]",
        errno);
    exit(2);
  }
  InitWakeupEvent();
  InitTimer();
  AddEpollEvent(wakeup_fd_event_);
  RPC_INFO_LOG("successful create wakeup event loop in thread %d", thread_id_);
  current_event_loop.reset(this);
}

EventLoop::~EventLoop() {
		// std::cout << "析构EventLoop" << std::endl;
  close(epoll_fd_);
  if (wakeup_fd_event_) {
    delete wakeup_fd_event_;
    wakeup_fd_event_ = nullptr;
  }
  if (timer_ != nullptr) {
    delete timer_;
    timer_ = nullptr;
  }
  // RPC_INFO_LOG("~EventLoop");
}

void EventLoop::InitTimer() {
  timer_ = new Timer();
  AddEpollEvent(timer_);
}

void EventLoop::InitWakeupEvent() {
  wakeup_fd_ = eventfd(0, EFD_NONBLOCK);
  if (wakeup_fd_ < 0) {
    RPC_ERROR_LOG(
        "failed to create event loop, wakeup_fd create error, error info[%d]",
        errno);
    exit(3);
  }
  wakeup_fd_event_ = new WakeUpFdEvent(wakeup_fd_);
  wakeup_fd_event_->Listen(FdEvent::IN_EVENT, [this]() {
    char buff[8];
    while (read(wakeup_fd_, buff, 8) != -1 && errno != EAGAIN)
      ;
    RPC_DEBUG_LOG("read full bytes from wakeup fd[%d]", wakeup_fd_);
  });
  RPC_DEBUG_LOG("create wake up fd, fd = %d", wakeup_fd_);
}

void EventLoop::AddTimerEvent(TimerEvent::s_ptr event) {
  timer_->AddTimerEvent(event);
}

void EventLoop::DeleteTimerEvent(TimerEvent::s_ptr event) {
  timer_->DeleteTimerEvent(event);
}

void EventLoop::Loop() {
  is_stop_ = false;
  while (true) {
    std::unique_lock<std::mutex> lock(latch_);
    std::queue<std::function<void()>> task_queue;
    task_queue.swap(task_queue_);
    lock.unlock();
    while (!task_queue.empty()) {
      task_queue.front()();
      task_queue.pop();
    }
    if (is_stop_) {
      return;
    }
    int timeout = epoll_max_timeout;
    epoll_event result_events[epoll_max_events];
    for (int i = 0; i < epoll_max_events; ++i) {
      result_events[i].events = 0;
    }
    RPC_DEBUG_LOG("begin to wait");
    int rt = epoll_wait(epoll_fd_, result_events, epoll_max_events, timeout);
    RPC_DEBUG_LOG("end  epoll_wait rt= %d", rt);
    if (rt < 0) {
      RPC_ERROR_LOG("epoll_wait error, error info[%s]", strerror(errno));
			std::cout << errno << strerror(errno) << std::endl;
			if (errno == 4) {
				continue;
			}
    } else {
      for (int i = 0; i < rt; ++i) {
        auto trigger_event = result_events[i];
        auto fd_event = static_cast<FdEvent *>(trigger_event.data.ptr);
        if (fd_event == nullptr) {
          continue;
        }
        if (trigger_event.events & EPOLLERR) {
          DeleteEpollEvent(fd_event);
          RPC_DEBUG_LOG("fd[%d] trigger EPOLLERR", fd_event->GetFd());
          if (fd_event->Handler(FdEvent::ERR_EVENT) != nullptr) {
            AddTask(fd_event->Handler(FdEvent::OUT_EVENT),false);
          }
        }
        if (trigger_event.events & EPOLLHUP) {
          DeleteEpollEvent(fd_event);
          RPC_DEBUG_LOG("fd[%d] trigger EPOLLHUP", fd_event->GetFd());
          if (fd_event->Handler(FdEvent::ERR_EVENT) != nullptr) {
            AddTask(fd_event->Handler(FdEvent::OUT_EVENT),false);
          }
        }
        if (trigger_event.events & EPOLLIN) {
          RPC_DEBUG_LOG("fd[%d] trigger EPOLLIN", fd_event->GetFd())
          AddTask(fd_event->Handler(FdEvent::IN_EVENT),false);
        }
        if (trigger_event.events & EPOLLOUT) {
          RPC_DEBUG_LOG("fd[%d] trigger EPOLLOUT", fd_event->GetFd())
          AddTask(fd_event->Handler(FdEvent::OUT_EVENT),false);
        }
      }
    }
  }
}

void EventLoop::WakeUp() {
  RPC_INFO_LOG("wake up fd=%d", wakeup_fd_);
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

    RPC_INFO_LOG("epoll_ctl[%d] event events[%d],fd = %d", op, (int)tmp.events,
                 event->GetFd());

    if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
      RPC_ERROR_LOG(
          "failed epoll_ctl when add fd %d,epoll_fd =[%d], error info[%d] =%s",
          event->GetFd(), epoll_fd_, errno, strerror(errno));
      return;
    }
    listen_fds_.insert(event->GetFd());
    RPC_DEBUG_LOG("add event success, fd[%d]", event->GetFd());
  } else {
    auto cb = [this, event]() {
      int op = EPOLL_CTL_ADD;
      if (listen_fds_.find(event->GetFd()) != listen_fds_.end()) {
        op = EPOLL_CTL_MOD;
      }
      epoll_event tmp = event->GetEpollEvent();

      RPC_INFO_LOG("epoll_ctl[%d] event events[%d],fd = %d", op,
                   (int)tmp.events, event->GetFd());

      if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
        RPC_ERROR_LOG("failed epoll_ctl when add fd %d, error info[%d] = %s",
                      event->GetFd(), errno, strerror(errno));
        return;
      }
      listen_fds_.insert(event->GetFd());
      RPC_DEBUG_LOG("add event success, fd[%d]", event->GetFd());
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

    RPC_INFO_LOG("epoll_ctl[%d] event events[%d],fd = %d", op, (int)tmp.events,
                 event->GetFd());

    if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
      RPC_ERROR_LOG("failed epoll_ctl when delete fd %d, error info[%d] = %s",
                    event->GetFd(), errno, strerror(errno));
    }
    listen_fds_.erase(event->GetFd());
    close(event->GetFd());
    RPC_DEBUG_LOG("```````close fd %d `````", event->GetFd());

    RPC_DEBUG_LOG("delete event success, fd[%d]", event->GetFd());
  } else {
    auto cb = [this, event] {
      if (listen_fds_.find(event->GetFd()) == listen_fds_.end()) {
        return;
      }
      int op = EPOLL_CTL_DEL;
      epoll_event tmp = event->GetEpollEvent();

      RPC_INFO_LOG("epoll_ctl[%d] event events[%d],fd = %d", op,
                   (int)tmp.events, event->GetFd());

      if ((epoll_ctl(epoll_fd_, op, event->GetFd(), &tmp)) == -1) {
        RPC_ERROR_LOG("failed epoll_ctl when delete fd %d, error info[%d] = ",
                      event->GetFd(), errno, strerror(errno));
      }
      listen_fds_.erase(event->GetFd());
      RPC_DEBUG_LOG("delete event success, fd[%d]", event->GetFd());

      close(event->GetFd());
      RPC_DEBUG_LOG("```````close fd %d `````", event->GetFd());
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
  if (current_event_loop == nullptr) {
    new EventLoop();
  }
  return current_event_loop.get();
}
} // namespace rocket
