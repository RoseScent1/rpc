#pragma once

#include "abstract_protocol.h"
#include <cstdint>
#include <memory>
#include <string>
namespace rocket {
class TinyPBProtocol : public AbstractProtocol {
public:
  using s_ptr = std::shared_ptr<TinyPBProtocol>;

	~TinyPBProtocol();
  static const char PB_START;
  static const char PB_END;
  void SetErrInfo(int32_t err_code, const char *err_info);

public:
  int32_t pk_len_{0};
  int32_t method_name_len_{0};
  std::string method_name_;
  int32_t err_code_{0};
  int32_t err_info_len_{0};
  std::string err_info_;
  std::string pb_data_;
  int32_t check_sum_{0};
  bool parse_success_{true};
};
} // namespace rocket