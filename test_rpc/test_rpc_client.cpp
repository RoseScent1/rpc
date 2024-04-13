
#include "config.h"
#include "event_loop.h"
#include "fd_event_group.h"
#include "log.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <google/protobuf/any.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "net_addr.h"
#include "order.pb.h"
#include "rpc_channel.h"
#include "rpc_closure.h"
#include "rpc_controller.h"
#include "tcp_client.h"
#include "util.h"

std::mutex latch_put;
int cnt = 0;
void make_order() {
  NEW_CHANNEL(channel, "127.0.0.1:8086");
  int err_num{0};
  NEW_CONTROLLER(controller);
  NEW_MESSAGE(makeOrderRequest, request);
  NEW_MESSAGE(makeOrderResponse, response);
  auto closure = std::make_shared<rocket::RpcClosure>(
      [&channel, request, response, &err_num]() {
        if (channel->GetController()->IsCanceled()) {
          ++err_num;
          // auto controller =
          //     dynamic_cast<rocket::RpcController
          //     *>(channel->GetController());
          // APP_INFO_LOG("call rpc failed,error code = %d, error info = %s",
          //              controller->GetErrorCode(),
          //              controller->GetErrorInfo().c_str());

        } else {
          // APP_INFO_LOG("call rpc success, request:%s ,response:%s",
          //              request->ShortDebugString().c_str(),
          //              response->ShortDebugString().c_str());
          if (response->order_id() != "123456") {
            ++err_num;
            // TODO: 业务逻辑
          }
        }
        channel->GetClient()->Stop();
      });
  controller->SetTimeOut(1000);
  request->set_price(1000);
  request->set_goods("cwl");
  for (int i = 0; i < 1000; ++i) {
    CALLRPC(controller, request, response, closure, channel, makeOrder);
  }
	
  std::unique_lock<std::mutex> lock(latch_put);
  if (err_num == 0)
    ++cnt;
}

void sum_order() {
  NEW_CHANNEL(channel, "127.0.0.1:8086");
  int err_num{0};
  NEW_CONTROLLER(controller);
  NEW_MESSAGE(sumRequest, request);
  NEW_MESSAGE(valResponse, response);
  auto closure = std::make_shared<rocket::RpcClosure>(
      [&channel, request, response, &err_num]() {
        if (channel->GetController()->IsCanceled()) {
          ++err_num;
          // auto controller =
          //     dynamic_cast<rocket::RpcController
          //     *>(channel->GetController());
          // APP_INFO_LOG("call rpc failed,error code = %d, error info = %s",
          //              controller->GetErrorCode(),
          //              controller->GetErrorInfo().c_str());
        } else {
          // APP_INFO_LOG("call rpc success, request:%s ,response:%s",
          //              request->ShortDebugString().c_str(),
          //              response->ShortDebugString().c_str());
          if (response->val() != 1002) {
            ++err_num;
          }
        }
        channel->GetClient()->Stop();
      });
  controller->SetTimeOut(1000);
  request->set_a(1000);
  request->set_b(2);
  for (int i = 0; i < 1000; ++i) {
    CALLRPC(controller, request, response, closure, channel, sumOrder);
  }
  std::unique_lock<std::mutex> lock(latch_put);
  if (err_num == 0)
    ++cnt;
}
int main() {

  rocket::Config::SetGlobalConfig(nullptr);
  // rocket::Config::GetGlobalConfig()->flag_ = true;
  rocket::Logger::InitGlobalLogger();
  rocket::FdEventGroup::GetFdEventGRoup();

  std::vector<std::thread> t;
  for (int i = 0; i < 20; ++i) {

    t.emplace_back(make_order);

    t.emplace_back(sum_order);
  }

  for (auto &i : t) {
    i.join();
  }
  // make_order(channel);
  rocket::Logger::GetGlobalLogger()->Stop();
	std::cout << cnt << std::endl;
  return 0;
}