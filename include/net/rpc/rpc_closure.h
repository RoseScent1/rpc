#pragma once
#include <functional>
#include <google/protobuf/stubs/callback.h>
namespace rocket {
class RpcClosure : public google::protobuf::Closure {
public:
  RpcClosure(std::function<void()> cb);
	~RpcClosure();
  void Run() override;

private:
  std::function<void()> callback_;
};
} // namespace rocket