#include "rpc_dispatcher.h"
#include "error_code.h"
#include "log.h"
#include "rpc_controller.h"
#include "tinypb_protocol.h"
#include <algorithm>
#include <cstddef>
#include <google/protobuf/arena.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <memory>
#include <type_traits>
namespace rocket {

static std::unique_ptr<RpcDispatcher> g_rpc_dispatcher = nullptr;
RpcDispatcher *RpcDispatcher::GetRpcDispatcher() {
  if (g_rpc_dispatcher == nullptr) {
    g_rpc_dispatcher.reset(new RpcDispatcher());
  }
  return g_rpc_dispatcher.get();
}

void RpcDispatcher::Dispatch(AbstractProtocol::s_ptr request,
                             AbstractProtocol::s_ptr response) {
  auto req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
  auto rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);

  rsp_protocol->msg_id_ = req_protocol->msg_id_;
  rsp_protocol->method_name_ = req_protocol->method_name_;
  std::string method_full_name = req_protocol->method_name_;
  std::string service_name;
  std::string method_name;

  if (!ParseServiceFullName(method_full_name, service_name, method_name)) {
    rsp_protocol->SetErrInfo(ERROR_PARSE_SERVICE_NAME,
                             "parse service name error");
    APP_ERROR_LOG("msg_id = [%d], call method name = [%s] failed, [can not "
                  "parse service name]",
                  req_protocol->msg_id_, method_full_name.c_str());
    return;
  }
  APP_INFO_LOG("msg_id = [%d], call method name = [%s]", req_protocol->msg_id_,
               method_full_name.c_str());

  auto it = service_table_.find(service_name);

  it = service_table_.find("Order");
  if (it == service_table_.end()) {
    APP_ERROR_LOG(
        "msg_id = [%d], call method name = [%s] failed, [service not found]",
        req_protocol->msg_id_, method_full_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_SERVICE_NOT_FOUND, "service not found");
    return;
  }

  auto service = it->second;
  auto method = service->GetDescriptor()->FindMethodByName(method_name);
  if (method == nullptr) {
    APP_ERROR_LOG(
        "msg_id = [%d], call method name = [%s] failed, [method not found]",
        req_protocol->msg_id_, method_full_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_SERVICE_NOT_FOUND, "method not found");
    return;
  }

  // 请求
  std::unique_ptr<google::protobuf::Message> req_msg(
      service->GetRequestPrototype(method).New());
  //  将pb_data反序列化为request
  if (!req_msg->ParseFromString(req_protocol->pb_data_)) {

    APP_ERROR_LOG("msg_id = [%d], call method name = [%s] failed, [request "
                  "deserialize failed]",
                  req_protocol->msg_id_, method_full_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_FAILED_DESERIALIZE, "deserialize error");
    return;
  }
  // 构造返回对象
  std::unique_ptr<google::protobuf::Message> rsp_msg(
      service->GetResponsePrototype(method).New());

  RpcController controller;
  controller.SetMsgId(req_protocol->msg_id_);

  service->CallMethod(
      method, &controller, req_msg.get(), rsp_msg.get(),
      nullptr); // nullptr 后续可以根据需要自己添加,具体实现与RpcController有关

  if (!rsp_msg->SerializeToString(&(rsp_protocol->pb_data_))) {

    APP_ERROR_LOG("msg_id = [%d], call method name = [%s] failed, [response "
                  "serialize failed]",
                  req_protocol->msg_id_, method_full_name.c_str());
    rsp_protocol->SetErrInfo(ERROR_FAILED_SERIALIZE, "serialize error");
    return;
  }
  rsp_protocol->err_code_ = 0;

  APP_ERROR_LOG("msg_id = [%d], call method name = [%s], [wait to send], "
                "request:[%s], response:[%s]",
                req_protocol->msg_id_, method_full_name.c_str(),
                req_msg->ShortDebugString().c_str(),
                rsp_msg->ShortDebugString().c_str());
}

bool RpcDispatcher::ParseServiceFullName(const std::string &full_name,
                                         std::string &service_name,
                                         std::string &method_name) {
  if (full_name.empty()) {
    RPC_ERROR_LOG("full_name is empty");
    return false;
  }
  auto pos = full_name.find_first_of(".");
  if (pos == full_name.npos) {
    RPC_ERROR_LOG("not find . in full name[%s]", full_name.c_str());
    return false;
  }
  service_name = full_name.substr(0, pos);
  method_name = full_name.substr(pos + 1, full_name.length() - pos - 1);

  return true;
}

void RpcDispatcher::RegisterService(
    std::shared_ptr<google::protobuf::Service> service) {
  std::string service_name = service->GetDescriptor()->full_name();
  service_table_.insert({service_name, service});
}
} // namespace rocket