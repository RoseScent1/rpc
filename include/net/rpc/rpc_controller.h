#pragma once

#include <cstdint>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>
namespace rocket {
class RpcController : public google::protobuf::RpcController {
public:
  using s_ptr = std::shared_ptr<RpcController>;

  RpcController() = default;
  ~RpcController() = default;

  void Reset() override;

  bool Failed() const override;

  std::string ErrorText() const override;

  void StartCancel() override;

  void SetFailed(const std::string &reason) override;

  bool IsCanceled() const override;

  void NotifyOnCancel(google::protobuf::Closure *callback) override;

  void SetError(int32_t error_code, const std::string error_info);
  int32_t GetErrorCode();
  std::string GetErrorInfo();
  void SetReqId(const std::string id);
  std::string GetReqId();
  // void SetLocalAddr(NetAddr::s_ptr addr);
  // void SetPeerAddr(NetAddr::s_ptr addr);
  // NetAddr::s_ptr GetLocalAddr();
  // NetAddr::s_ptr GetPeerAddr();

  void SetTimeOut(int timeout);
  int GetTimeOut();

private:
  int error_code_{0};
  std::string error_info_;
  std::string req_id_;

  bool is_failed_{false};
  bool is_canceled_{false};

  // NetAddr::s_ptr local_addr_;
  // NetAddr::s_ptr peer_addr_;

  int timeout_{1000}; // ms
};
} // namespace rocket