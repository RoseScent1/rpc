#pragma once

#include "abstract_protocol.h"
#include "tcp_buffer.h"
#include <memory>
#include <vector>
namespace rocket {
class AbstractCoder {
public:
  using s_ptr = std::shared_ptr<AbstractCoder>;
  virtual void EnCode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &out_buffer) = 0;
  virtual void Decode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &out_buffer) = 0;
  virtual ~AbstractCoder() = default;
};
} // namespace rocket