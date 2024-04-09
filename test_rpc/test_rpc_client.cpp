
#include "config.h"
#include "log.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <google/protobuf/service.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net_addr.h"
#include "order.pb.h"
#include "rpc_dispatcher.h"
#include "tcp_client.h"
#include "tcp_server.h"
#include "tinypb_protocol.h"

void test_client() {
  std::string s("127.0.0.1:8086");
  auto a = std::make_shared<rocket::IPNetAddr>(s);
  rocket::TcpClient client(a);
  client.Connect([&client, a]() {
    INFOLOG("client connect to server[%s]", a->ToString().c_str());
    auto message = std::make_shared<rocket::TinyPBProtocol>();

    message->req_id_ = "999";

    makeOrderRequest request;
    request.set_price(100);
    request.set_goods("apple");
    message->method_name_ = "Order.makeOrder";
    if (!request.SerializeToString(&(message->pb_data_))) {
      ERRORLOG("client serializer error")
      return;
    }
    client.WriteMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("write message success");
    });
    client.ReadMessage("999", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      auto message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      DEBUGLOG("read req_ie = [%s] get response success,pb_data = %s",
               msg_ptr->req_id_.c_str(), message->pb_data_.c_str());
      makeOrderResponse response;
      if (!response.ParseFromString(message->pb_data_)) {
        ERRORLOG("client deserializer error")
        return;
      }
      DEBUGLOG("get response success, response[%s]",
               response.ShortDebugString().c_str());
    });
    client.WriteMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("write message success");
    });
    client.ReadMessage("999", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      auto message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      DEBUGLOG("read req_ie = [%s] get response success,pb_data = %s",
               msg_ptr->req_id_.c_str(), message->pb_data_.c_str());
      makeOrderResponse response;
      if (!response.ParseFromString(message->pb_data_)) {
        ERRORLOG("client deserializer error")
        return;
      }
      DEBUGLOG("get response success, response[%s]",
               response.ShortDebugString().c_str());
    });
  });
}
int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();

  test_client();
  return 0;
}