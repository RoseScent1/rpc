#include "config.h"
#include "eventloop.h"
#include "log.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
int main() {
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  std::vector<std::thread> thread;
  rocket::EventLoop *eventloop = new rocket::EventLoop();
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
	eventloop->AddEpollEvent(&event);
	eventloop->Loop();
}