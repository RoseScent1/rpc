#include "io_thread.h"
#include "event_loop.h"
#include "log.h"
#include "util.h"
#include <semaphore.h>
#include <thread>

namespace rocket {
IOThread::IOThread() {
  if (sem_init(&sem_init_, 0, 0) != 0) {
    ERRORLOG("sem init error");
    exit(0);
  }
  if (sem_init(&sem_start_, 0, 0) != 0) {
    ERRORLOG("sem init error");
    exit(0);
  }
  thread_ = std::thread(Main, this);
  sem_wait(&sem_init_);
  DEBUGLOG("IOThread create successful threadId=%d", thread_id_);
}

IOThread::~IOThread() {
  INFOLOG("IOThread deleted threadid = %d", thread_id_);
  event_loop_->Stop();
  sem_destroy(&sem_init_);
  sem_destroy(&sem_start_);
  delete event_loop_;
  event_loop_ = nullptr;
}

EventLoop *IOThread::GetEventloop() { return event_loop_; }

void IOThread::Start() {
  DEBUGLOG("now invoke IOThread");
  sem_post(&sem_start_);
}

void IOThread::Join() { thread_.join(); }

void IOThread::Main(IOThread *thread) {

  thread->event_loop_ = new EventLoop();
  thread->thread_id_ = getThreadid();
  sem_post(&(thread->sem_init_));
  sem_wait(&thread->sem_start_);
  DEBUGLOG("start IOthread eventloop threadid = %d", thread->thread_id_);
  thread->event_loop_->Loop();
  DEBUGLOG("end IOthread threadid = %d", thread->thread_id_);
}

} // namespace rocket
