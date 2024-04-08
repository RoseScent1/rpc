#include "rpc_dispatcher.h"
#include "error_code.h"
#include "log.h"
#include "net_addr.h"
#include "rpc_controller.h"
#include "tinypb_protocol.h"
#include <algorithm>
#include <google/protobuf/arena.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <memory>
#include <type_traits>
namespace rocket {

void RpcDispatcher::Dispatch(AbstractProtocol::s_ptr request,
                             AbstractProtocol::s_ptr response) {
  auto req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
  auto rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);

  req_protocol->req_id_ = req_protocol->req_id_;
  req_protocol->method_name_ = req_protocol->method_name_;

  std::string method_full_name = req_protocol->method_name_;
  std::string service_name;
  std::string method_name;

  if (ParseServiceFullName(method_full_name, service_name, method_name)) {
    rsp_protocol->SetErrInfo(ERROR_PARSE_SERVICE_NAME,
                             "parse service name error");
    return;
  }

  auto it = service_table_.find(service_name);
  if (it == service_table_.end()) {
    ERRORLOG("req_id = %s, service %s not found", req_protocol->req_id_.c_str(),
             service_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_SERVICE_NOT_FOUND, "service not found");
    return;
  }

  auto service = it->second;
  std::unique_ptr<const google::protobuf::MethodDescriptor> method(
      service->GetDescriptor()->FindMethodByName(method_name));
  if (method == nullptr) {
    ERRORLOG("req id = %s,service %s method %s not found",
             req_protocol->req_id_.c_str(), service_name.c_str(),
             method_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_SERVICE_NOT_FOUND, "method not found");
    return;
  }

  // 请求
  std::unique_ptr<google::protobuf::Message> req_msg(
      service->GetRequestPrototype(method.get()).New());
  //  将pb_data反序列化为request
  if (!req_msg->ParseFromString(req_protocol->pb_data_)) {
    ERRORLOG("request deserialize error,req id = %s,service %s method %s ",
             req_protocol->req_id_.c_str(), service_name.c_str(),
             method_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_FAILED_DESERIALIZE, "deserialize error");
    return;
  }
  INFOLOG("get rpc request %s, req id = %s", req_msg->ShortDebugString().c_str(),
          req_protocol->req_id_.c_str());

  // 构造返回对象
  std::unique_ptr<google::protobuf::Message> rsp_msg(
      service->GetResponsePrototype(method.get()).New());

  RpcController controller;
  controller.SetReqId(req_protocol->req_id_);

  service->CallMethod(method.get(), &controller, req_msg.get(), rsp_msg.get(),
                      nullptr); // 未完全实现 nullptr需要实现

  if (rsp_msg->SerializeToString(&(req_protocol->pb_data_))) {
    ERRORLOG("response serialize error,req id = %s,message = %s",
             req_protocol->req_id_.c_str(), rsp_msg->DebugString().c_str());
    rsp_protocol->SetErrInfo(ERROR_FAILED_SERIALIZE, "serialize error");
    return;
  }
  rsp_protocol->err_code_ = 0;
  INFOLOG("req id %s,service %s, method %s success dispatch",
          req_protocol->req_id_.c_str(), req_msg->ShortDebugString().c_str(),
          rsp_msg->ShortDebugString().c_str());
}

bool RpcDispatcher::ParseServiceFullName(const std::string &full_name,
                                         std::string &service_name,
                                         std::string &method_name) {
  if (full_name.empty()) {
    ERRORLOG("full_name is empty");
    return false;
  }
  auto pos = full_name.find_first_of(".");
  if (pos == full_name.npos) {
    ERRORLOG("not find . in full name[%s]", full_name.c_str());
    return false;
  }
  service_name = full_name.substr(0, pos);
  method_name = full_name.substr(pos + 1, full_name.length() - pos - 1);

  INFOLOG("parse service name[%s] and method name[%s]", service_name.c_str(),
          method_name.c_str());
  return true;
}

void RpcDispatcher::RegisterService(
    std::shared_ptr<google::protobuf::Service> service) {
  std::string service_name = service->GetDescriptor()->full_name();
  service_table_[service_name] = service;
}
} // namespace rocket