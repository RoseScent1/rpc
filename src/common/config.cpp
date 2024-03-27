#include <config.h>
#include <cstddef>
#include <iostream>
#include <memory>
#include <tinyxml/tinyxml.h>
namespace rocket {

static Config *g_config = nullptr;
Config *Config::GetGlobalConfig() { return g_config; }

void Config::SetGlobalConfig(const char *xmlfile) {
  g_config = new Config(xmlfile);
}
Config::Config(const char *xmlfile) {
  auto xml_document = std::make_shared<TiXmlDocument>();
  auto read_check = [&xml_document](bool flag, const char *s) {
    if (flag) {
      std::cerr << "Start rocket error, failed to read  " << s << std::endl;
      std::cerr << "ErrorInfo:" << xml_document->ErrorDesc() << std::endl;
      exit(0);
    }
  };
  // 读文件
  bool rt = xml_document->LoadFile(xmlfile);
  read_check(!rt, "config file");

  // 读root结点
  auto root_node = xml_document->FirstChildElement("root");
  read_check(root_node == nullptr, "node");
  // 读log结点
  auto log_node = root_node->FirstChildElement("log");
  read_check(log_node == nullptr, "log");

  auto log_level_node = log_node->FirstChildElement("log_level");
  read_check(log_level_node == nullptr || log_level_node->GetText() == nullptr,
             "log_level");
  log_level_ = log_level_node->GetText();
}

} // namespace rocket