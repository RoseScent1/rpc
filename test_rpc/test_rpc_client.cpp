
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
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "net_addr.h"
#include "order.pb.h"
#include "rpc_channel.h"
#include "rpc_closure.h"
#include "rpc_controller.h"
#include "rpc_dispatcher.h"
#include "tcp_client.h"
#include "tcp_server.h"
#include "tinypb_protocol.h"
#include "util.h"

void test_client() {
  std::string s("172.18.10.174:8086");
  auto a = std::make_shared<rocket::IPNetAddr>(s);
  rocket::TcpClient client(a);
  client.Connect([&client, a]() {
    INFOLOG("client connect to server[%s]", a->ToString().c_str());
    auto message = std::make_shared<rocket::TinyPBProtocol>();

    message->msg_id_ = "999";

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
               msg_ptr->msg_id_.c_str(), message->pb_data_.c_str());
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

void test_channel() {
  rocket::IPNetAddr::s_ptr addr =
      std::make_shared<rocket::IPNetAddr>("172.18.10.174", 8086);
  auto channel = std::make_shared<rocket::RpcChannel>(addr);
  auto request = std::make_shared<makeOrderRequest>();
  request->set_price(1);
  request->set_goods("ysl");

  auto controller = std::make_shared<rocket::RpcController>();

  auto response = std::make_shared<makeOrderResponse>();
	
  auto closure =
      std::make_shared<rocket::RpcClosure>([&channel, request, response]() {
        INFOLOG("closure callback run\n request:%s ,response:%s",
                request->ShortDebugString().c_str(),
                response->ShortDebugString().c_str());
        INFOLOG("new stop event loop");
        channel->GetClient()->Stop();
      });
  channel->Init(controller, request, response, closure);

  Order_Stub stub(channel.get());
  stub.makeOrder(controller.get(), request.get(), response.get(),
                 closure.get());
	if (controller->Failed() != 0) {
		ERRORLOG("makeOrder error, error code = %d, error info = %s",controller->GetErrorCode(),controller->GetErrorInfo().c_str());
	}
  INFOLOG("channel s_ptr use count = %d", channel.use_count());
}
int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  test_channel();
  return 0;
}