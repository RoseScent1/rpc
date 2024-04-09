#include "config.h"
#include "log.h"
#include "net_addr.h"
#include "tcp_server.h"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

void test() {
  std::string s("172.18.10.174:8086");
  auto a = std::make_shared<rocket::IPNetAddr>(s);
  rocket::TcpServer tcp_server(a);
  tcp_server.start();
}

int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
	test();
}