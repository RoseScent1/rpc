#pragma once

#include <memory>
#include <string>
namespace rocket {

struct AbstractProtocol {
public:
  using s_ptr = std::shared_ptr<AbstractProtocol>;

  virtual ~AbstractProtocol() = default;


public:
  std::string msg_id_; // 请求号，唯一标识一个请求或者响应

};
} // namespace rocket