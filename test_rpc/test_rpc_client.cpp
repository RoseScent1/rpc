
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
#include "tcp_client.h"


void test_channel() {
  NEW_CHANNEL(channel, "127.0.0.1:8086");
  NEW_CONTROLLER(controller);
  NEW_MESSAGE(makeOrderRequest, request);
  NEW_MESSAGE(makeOrderResponse, response);
  auto closure = std::make_shared<rocket::RpcClosure>([&channel, request,
                                                       response]() {
    if (channel->GetController()->IsCanceled()) {
      auto controller =
          dynamic_cast<rocket::RpcController *>(channel->GetController());
      APP_INFO_LOG("call rpc failed,error code = %d, error info = %s",
              controller->GetErrorCode(), controller->GetErrorInfo().c_str());
    } else {
      APP_INFO_LOG("call rpc success, request:%s ,response:%s",
              request->ShortDebugString().c_str(),
              response->ShortDebugString().c_str());
    }
    RPC_INFO_LOG("now stop event loop");
    channel->GetClient()->Stop();
  });
  controller->SetTimeOut(1000);
  request->set_price(1000);
  request->set_goods("ysl");
  CALLRPC(controller, request, response, closure, channel, makeOrder);

  controller->SetTimeOut(1000);
  request->set_price(1000);
  request->set_goods("ysl");
  CALLRPC(controller, request, response, closure, channel, makeOrder);
}
int main() {
  rocket::Config::SetGlobalConfig("/home/cwl/Desktop/rpc/conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  test_channel();
	rocket::Logger::GetGlobalLogger()->Stop();
  return 0;
}