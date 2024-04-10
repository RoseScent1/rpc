#include "config.h"
#include "event_loop.h"
#include "io_thread.h"
#include "io_thread_group.h"
#include "net_addr.h"
#include "log.h"
#include "timer.h"
#include "timer_event.h"
#include "util.h"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

void test_iothread() {
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1) {
    ERRORLOG("listenfd = -1");
    exit(0);
  }
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_port = htons(12347);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
    DEBUGLOG("bind = -1");
    exit(0);
  }
  if (listen(listenfd, 100) == -1) {
    DEBUGLOG("listen = -1");
    exit(0);
  }
  rocket::FdEvent event(listenfd);
  event.Listen(rocket::FdEvent::IN_EVENT, [listenfd]() {
    sockaddr_in client_addr;
    socklen_t len = sizeof(addr);
    int clientfd =
        accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &len);
    DEBUGLOG("successful get client [%s:%d]", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));
  });
  int i = 0;

  auto timer_event = std::make_shared<rocket::TimerEvent>(
      10000, true, [&i]() { INFOLOG("trigger timer event, count = %d", ++i); });

  rocket::IOThreadGroup iothread_group(2);
  auto io_thread = iothread_group.GetIOThread();
  io_thread->GetEventloop()->AddEpollEvent(&event);
  io_thread->GetEventloop()->AddTimerEvent(timer_event);

  io_thread = iothread_group.GetIOThread();
  io_thread->GetEventloop()->AddTimerEvent(timer_event);

	iothread_group.Start();
	iothread_group.Join();
}
int main() {
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  test_iothread();
  // std::vector<std::thread> thread;
  // rocket::EventLoop *eventloop = new rocket::EventLoop();
  // int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  // if (listenfd == -1) {
  //   ERRORLOG("listenfd = -1");
  //   exit(0);
  // }
  // sockaddr_in addr;
  // memset(&addr, 0, sizeof(addr));
  // addr.sin_port = htons(12347);
  // addr.sin_family = AF_INET;
  // addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // if (bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
  // -1) {
  //   DEBUGLOG("bind = -1");
  //   exit(0);
  // }
  // if (listen(listenfd, 100) == -1) {
  //   DEBUGLOG("listen = -1");
  //   exit(0);
  // }
  // rocket::FdEvent event(listenfd);
  // event.Listen(rocket::FdEvent::IN_EVENT, [listenfd]() {
  //   sockaddr_in client_addr;
  //   socklen_t len = sizeof(addr);
  //   int clientfd =
  //       accept(listenfd, reinterpret_cast<sockaddr *>(&client_addr), &len);
  //   DEBUGLOG("successful get client [%s:%d]",
  //   inet_ntoa(client_addr.sin_addr),
  //            ntohs(client_addr.sin_port));
  // });
  // eventloop->AddEpollEvent(&event);
  // int i = 0;
  // auto timer_event = std::make_shared<rocket::TimerEvent>(
  //     1000, true, [&i]() { INFOLOG("trigger timer event, count = %d", ++i);
  //     });

  // eventloop->AddTimerEvent(timer_event);
  // eventloop->Loop();
}