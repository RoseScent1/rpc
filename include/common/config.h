#pragma once
#include <map>
#include <string>

namespace rocket {

class Config {
public:
  Config(const char *xmlfile);
  static Config *GetGlobalConfig();
  static void SetGlobalConfig(const char *xmlfile);


  std::string log_level_;
};

} // namespace rocket