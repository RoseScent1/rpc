
#include "config.h"
#include "fd_event_group.h"
#include "log.h"
#include "net_addr.h"
#include "order.pb.h"
#include "rpc_dispatcher.h"
#include "tcp_server.h"
#include <google/protobuf/service.h>
#include <iostream>
#include <memory>
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
    RPC_INFO_LOG("request price = %d, goods = %s", request->price(),
                 request->goods().c_str());
    response->set_order_id("123456");
  }
  void sumOrder(google::protobuf::RpcController *controller,
                const ::sumRequest *request, ::valResponse *response,
                ::google::protobuf::Closure *done) override {
    response->set_val(request->a() + request->b());
    response->set_res_info(666);
  }
};

int main(int argc, char *argv[]) {
  // if (argc != 2) {
  //   std::cout << "Start like this : ./rpc_server /home/conf/rocket.xml"
  //             << std::endl;
  // 	return 1;
  // }
// 初始化全局类
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
	rocket::FdEventGroup::GetFdEventGRoup();

  auto service = std::make_shared<OrderImpl>();
  rocket::RpcDispatcher::GetRpcDispatcher()->RegisterService(service);
  auto a = std::make_shared<rocket::IPNetAddr>(
      "127.0.0.1", rocket::Config::GetGlobalConfig()->port_);
  rocket::TcpServer tcp_server(a);
  tcp_server.start();
  return 0;
}