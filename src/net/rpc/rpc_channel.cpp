#include "rpc_channel.h"
#include "error_code.h"
#include "log.h"
#include "net_addr.h"
#include "rpc_controller.h"
#include "tcp_client.h"
#include "tinypb_protocol.h"
#include "util.h"
#include <cstddef>
#include <google/protobuf/arena.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/strutil.h>
#include <memory>
namespace rocket {

RpcChannel::RpcChannel(NetAddr::s_ptr addr)
    : client_(std::make_shared<TcpClient>(addr)) {}

RpcChannel::~RpcChannel() {
  // INFOLOG("~RpcChannel");
}
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done) {

  auto req_protocol = std::make_shared<TinyPBProtocol>();
  req_protocol->msg_id_ = GenMsgId();

  if (!is_init_) {
    ERRORLOG("not init RPC Channel");
    return;
  }
  req_protocol->msg_id_ = GenMsgId();
  req_protocol->method_name_ = method->full_name();
  INFOLOG("msg_id = [%s] call method name [%s]", req_protocol->msg_id_.c_str(),
          req_protocol->method_name_.c_str());

  // request 序列化
  if (!request->SerializeToString(&(req_protocol->pb_data_))) {
    ERRORLOG("msg_id = [%s] , failed to serializer ,error message = %s",
             req_protocol->msg_id_.c_str(),
             request->ShortDebugString().c_str());
    return;
  }

  // 构造客户端
  // TcpClient

  client_->Connect([req_protocol, this]() {
    auto my_controller = dynamic_cast<RpcController *>(GetController());
    if (client_->GetErrCode() != 0) {
      my_controller->SetError(client_->GetErrCode(), client_->GetErrInfo());
      ERRORLOG("msg-id = %s, connect error,error info[%s]",
               req_protocol->msg_id_.c_str(),
               my_controller->GetErrorInfo().c_str());
      return;
    }
    client_->WriteMessage(req_protocol, [my_controller, req_protocol,
                                         this](rocket::AbstractProtocol::s_ptr
                                                   msg_ptr) {
      INFOLOG("write success msg_id = [%s]", req_protocol->msg_id_.c_str());
      client_->ReadMessage(
          req_protocol->msg_id_,
          [my_controller, this](rocket::AbstractProtocol::s_ptr msg_ptr) {
            auto rsp_protocol =
                std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            if (my_controller == nullptr) {
              ERRORLOG("failed CallMethod RpcController convert error");
              return;
            }
            if (!GetResponse()->ParseFromString(rsp_protocol->pb_data_)) {
              ERRORLOG("response deserializer error");
              my_controller->SetError(ERROR_FAILED_DESERIALIZE,
                                      "response deserializer error");
              return;
            }
            if (rsp_protocol->err_code_ != 0) {
              ERRORLOG("response has error_code");
              my_controller->SetError(rsp_protocol->err_code_,
                                      rsp_protocol->err_info_);
              return;
            }
            INFOLOG(
                "read req_id = [%s] get response success,call method name= %s",
                msg_ptr->msg_id_.c_str(), rsp_protocol->method_name_.c_str());
            if (GetClosure()) {
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