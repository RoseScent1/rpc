#include "rpc_controller.h"
#include <google/protobuf/stubs/callback.h>

namespace rocket {

void RpcController::Reset() {
  error_code_ = 0;
  error_info_.clear();
  req_id_.clear();

  is_failed_ = false;
  is_canceled_ = false;

  // local_addr_.reset();
  // peer_addr_.reset();

  timeout_ = 1000; // ms
}

bool RpcController::Failed() const { return is_failed_; }

std::string RpcController::ErrorText() const { return error_info_; }

void RpcController::StartCancel() { is_canceled_ = true; }

void RpcController::SetFailed(const std::string &reason) {
  error_info_ = reason;
}
bool RpcController::IsCanceled() const { return is_canceled_; }

void RpcController::NotifyOnCancel(google::protobuf::Closure *callback) {}

void RpcController::SetError(int32_t error_code, const std::string error_info) {
  error_code_ = error_code;
  error_info_ = error_info;
}

int32_t RpcController::GetErrorCode() { return error_code_; }

std::string RpcController::GetErrorInfo() { return error_info_; }

void RpcController::SetReqId(const std::string id) { req_id_ = id; }

std::string RpcController::GetReqId() { return req_id_; }



void RpcController::SetTimeOut(int timeout) { timeout_ = timeout; }

int RpcController::GetTimeOut() { return timeout_; }


} // namespace rocket