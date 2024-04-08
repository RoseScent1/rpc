#include "rpc_closure.h"

namespace rocket {
void RpcClosure::Run() {
  if (callback_ != nullptr) {
    callback_();
  }
}
} // namespace rocket