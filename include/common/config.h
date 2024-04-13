#pragma once
#include <cstdint>
#include <map>
#include <string>

namespace rocket {

class Config {
public:
  Config(const char *xmlfile);
  Config();
  static Config *GetGlobalConfig();
  static void SetGlobalConfig(const char *xmlfile);

  std::string log_level_;
  std::string file_name_;
  std::string file_path_;
  uint32_t file_max_line_;
  uint32_t interval_{1000}; // 写入文件的间隔 ms
  uint32_t file_number_{0};
  uint16_t port_{0};
  uint16_t io_thread_{4};

public:
  bool flag_{false}; // false 客户端, true 服务端
};

} // namespace rocket