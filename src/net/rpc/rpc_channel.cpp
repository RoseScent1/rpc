#include "rpc_channel.h"
#include "error_code.h"
#include "log.h"
#include "net_addr.h"
#include "rpc_controller.h"
#include "tcp_client.h"
#include "timer_event.h"
#include "tinypb_protocol.h"
#include "util.h"
#include <cstddef>
#include <google/protobuf/arena.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/strutil.h>
#include <memory>
#include <string>
namespace rocket {

RpcChannel::RpcChannel(NetAddr::s_ptr addr)
    : client_(std::make_shared<TcpClient>(addr)) {}

RpcChannel::~RpcChannel() {
  // std::cout << "析构RpcChannel" << std::endl;
}
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done) {

  auto req_protocol = std::make_shared<TinyPBProtocol>();

  if (!is_init_) {
    RPC_ERROR_LOG("not init RPC Channel");
    return;
  }
  auto my_controller = dynamic_cast<RpcController *>(controller_.get());
  if (my_controller == nullptr) {
    RPC_ERROR_LOG("failed CallMethod RpcController convert error");
    return;
  }

  req_protocol->msg_id_ = GenMsgId();
  req_protocol->method_name_ = method->full_name();
  APP_INFO_LOG("msg_id = [%d] call method name [%s]", req_protocol->msg_id_,
               req_protocol->method_name_.c_str());

  // request 序列化
  if (!request->SerializeToString(&(req_protocol->pb_data_))) {
    RPC_ERROR_LOG("msg_id = [%d] , failed to serializer ,error message = %s",
                  req_protocol->msg_id_, request->ShortDebugString().c_str());
    return;
  }

  // 设置定时器
  timer_event_ = std::make_shared<TimerEvent>(
      my_controller->GetTimeOut(), false, [my_controller, this]() {
        my_controller->StartCancel();
        my_controller->SetError(
            ERROR_RPC_CALL_TIMEOUT,
            "rpc call timeout(ms): " +
                std::to_string(my_controller->GetTimeOut()));
        if (closure_ != nullptr) {
          closure_->Run();
        }
      });

  client_->GetEventLoop()->AddTimerEvent(timer_event_);
  // 构造客户端
  // TcpClient

  client_->Connect([req_protocol, this]() {
    auto my_controller = dynamic_cast<RpcController *>(GetController());
    if (client_->GetErrCode() != 0) {
      my_controller->SetError(client_->GetErrCode(), client_->GetErrInfo());
      APP_ERROR_LOG("msg_id = %d, connect error,error info[%s]",
                    req_protocol->msg_id_,
                    my_controller->GetErrorInfo().c_str());
      return;
    }
    client_->WriteMessage(
        req_protocol, [my_controller, req_protocol,
                       this](rocket::AbstractProtocol::s_ptr msg_ptr) {
          RPC_INFO_LOG("write success msg_id = [%d]", req_protocol->msg_id_);
          client_->ReadMessage(
              req_protocol->msg_id_,
              [my_controller, this](rocket::AbstractProtocol::s_ptr msg_ptr) {
                // 取消定时器
                client_->GetEventLoop()->DeleteTimerEvent(timer_event_);
                auto rsp_protocol =
                    std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
                if (!GetResponse()->ParseFromString(rsp_protocol->pb_data_)) {
                  RPC_ERROR_LOG("response deserializer error");
                  my_controller->SetError(ERROR_FAILED_DESERIALIZE,
                                          "response deserializer error");
                  return;
                }

                if (rsp_protocol->err_code_ != 0) {
                  RPC_ERROR_LOG("response has error_code");
                  my_controller->SetError(rsp_protocol->err_code_,
                                          rsp_protocol->err_info_);
                  return;
                }
                RPC_INFO_LOG("req_id = [%d] get response success",
                             msg_ptr->msg_id_);

                if (!controller_->IsCanceled() && GetClosure()) {
                  GetClosure()->Run();
                }
              });
        });
  });
}

void RpcChannel::Init(
    std::shared_ptr<google::protobuf::RpcController> controller,
    std::shared_ptr<google::protobuf::Message> request,
    std::shared_ptr<google::protobuf::Message> response,
    std::shared_ptr<google::protobuf::Closure> closure) {
  if (is_init_) {
    return;
  }
  controller_ = controller;
  request_ = request;
  response_ = response;
  closure_ = closure;
  is_init_ = true;
}

google::protobuf::RpcController *RpcChannel::GetController() const {
  return controller_.get();
}
google::protobuf::Message *RpcChannel::GetRequest() const {
  return request_.get();
}
google::protobuf::Message *RpcChannel::GetResponse() const {
  return response_.get();
}
google::protobuf::Closure *RpcChannel::GetClosure() const {
  return closure_.get();
}
TcpClient *RpcChannel::GetClient() const { return client_.get(); }
} // namespace rocket