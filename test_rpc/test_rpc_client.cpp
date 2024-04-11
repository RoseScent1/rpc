
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


void test_channel() {
  NEWCHANNEL(channel, "127.0.0.1:8086");
  NEWCONTROLLER(controller);
  NEWMESSAGE(makeOrderRequest, request);
  NEWMESSAGE(makeOrderResponse, response);
  auto closure = std::make_shared<rocket::RpcClosure>([&channel, request,
                                                       response]() {
    if (channel->GetController()->IsCanceled()) {
      auto controller =
          dynamic_cast<rocket::RpcController *>(channel->GetController());
      INFOLOG("call rpc failed,error code = %d, error info = %s",
              controller->GetErrorCode(), controller->GetErrorInfo().c_str());
    } else {
      INFOLOG("call rpc success, request:%s ,response:%s",
              request->ShortDebugString().c_str(),
              response->ShortDebugString().c_str());
    }
    INFOLOG("now stop event loop");
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
  return 0;
}