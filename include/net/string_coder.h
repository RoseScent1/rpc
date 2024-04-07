#pragma once

#include "abstract_coder.h"
#include "abstract_protocol.h"
#include <memory>
namespace rocket {
class StringProtocol : public AbstractProtocol {
public:
  void test() {}
  std::string info;
};

class Stringcoder : public AbstractCoder {
public:
  virtual void EnCode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &out_buffer) {
    for (auto &i : message) {
      StringProtocol *msg = dynamic_cast<StringProtocol *>(i.get());
      out_buffer->WriteToBuffer(msg->info.c_str(), msg->info.size());
    }
  }
  virtual void Decode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &in_buffer) {
    auto msg = std::make_shared<StringProtocol>();
    message.emplace_back(msg);
    in_buffer->ReadFromBuffer(msg->info, in_buffer->ReadAble());
    msg->SetReqId("123");
  }
};
} // namespace rocket