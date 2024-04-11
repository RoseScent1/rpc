#include "rpc_closure.h"
#include "log.h"
#include <functional>

namespace rocket {
RpcClosure::RpcClosure(std::function<void()> cb) : callback_(cb) {}
RpcClosure::~RpcClosure() {
  // RPC_INFO_LOG("~RpcClosure");
}
void RpcClosure::Run() {
  if (callback_ != nullptr) {
    callback_();
  }
}
} // namespace rocket