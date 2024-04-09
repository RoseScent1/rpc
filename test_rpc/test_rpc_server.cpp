
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
#include "tcp_server.h"
class OrderImpl : public Order {
public:
  void makeOrder(google::protobuf::RpcController *controller,
                 const ::makeOrderRequest *request,
                 ::makeOrderResponse *response,
                 ::google::protobuf::Closure *done) override {

    if (request->price() < 10) {
      response->set_ret_code(-1);
      response->set_res_info("not enough money");
      return;
    }
		INFOLOG("request price = %d, goods = %s",request->price(),request->goods().c_str());
    response->set_order_id("123456");
  }
};

void test() {
  std::string s("127.0.0.1:8086");
  auto a = std::make_shared<rocket::IPNetAddr>(s);
  rocket::TcpServer tcp_server(a);
  tcp_server.start();
}

int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();

  auto service = std::make_shared<OrderImpl>();
  rocket::RpcDispatcher::GetRpcDispatcher()->RegisterService(service);
  test();
  return 0;
}