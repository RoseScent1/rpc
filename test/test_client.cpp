#include "abstract_protocol.h"
#include "config.h"
#include "log.h"
#include "tcp_client.h"
#include "tinypb_protocol.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
void test_connection() {

  int client_fd = socket(PF_INET, SOCK_STREAM, 0);

  sockaddr_in ser_addr{};
  ser_addr.sin_family = AF_INET;
  ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  ser_addr.sin_port = htons(8086);
  socklen_t len = sizeof(ser_addr);
  if (connect(client_fd, reinterpret_cast<sockaddr *>(&ser_addr), len) == -1)
    return;
  std::string s = "hello";
  write(client_fd, s.c_str(), 5);
  DEBUGLOG("client write success");
  s = "1234567";
  read(client_fd, &s[0], 5);
  DEBUGLOG("read %s", s.c_str());
  close(client_fd);
}

void test_client() {
  std::string s("127.0.0.1:8086");
  auto a = std::make_shared<rocket::IPNetAddr>(s);
  rocket::TcpClient client(a);
  client.Connect([&client, a]() {
    INFOLOG("client connect to server[%s]", a->ToString().c_str());
    auto message = std::make_shared<rocket::TinyPBProtocol>();
    message->pb_data_ = ("test pb data");
    message->req_id_ = "123";
    client.WriteMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("write message sucess111");
    });
    client.ReadMessage("123", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      auto message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      DEBUGLOG("read req_ie = [%s] message sucess,pb_data = %s", msg_ptr->req_id_.c_str(),message->pb_data_.c_str());
    });
  });
}
int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  test_client();
}