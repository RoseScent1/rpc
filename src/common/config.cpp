#include "log.h"
#include <cassert>
#include <config.h>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <tinyxml/tinyxml.h>
namespace rocket {

#define read_xml(name)                                                         \
  auto name = log_node->FirstChildElement(#name);                              \
  read_check(name == nullptr || name->GetText() == nullptr, #name);
static std::unique_ptr<Config> g_config = nullptr;
Config *Config::GetGlobalConfig() { return g_config.get(); }

void Config::SetGlobalConfig(const char *xmlfile) {
  if (xmlfile == nullptr) {
    g_config.reset(new Config());
  } else {
    g_config.reset(new Config(xmlfile));
  }
}
Config::Config() { log_level_ = Debug; }
Config::Config(const char *xmlfile) {
  flag_ = true;
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

  read_xml(log_level);
  read_xml(file_name);
  read_xml(file_path);
  read_xml(file_max_line);
  read_xml(interval);
  read_xml(file_number);

  auto server_node = root_node->FirstChildElement("server");
  read_check(server_node == nullptr, "log");

  auto port = server_node->FirstChildElement("port");
  read_check(port == nullptr, "port");
  auto io_thread = server_node->FirstChildElement("io_thread");
  read_check(io_thread == nullptr, "io_thread");

  log_level_ = log_level->GetText();
  file_name_ = file_name->GetText();
  file_path_ = file_path->GetText();
  file_max_line_ = atoi(file_max_line->GetText());
  interval_ = atoi(interval->GetText());
  file_number_ = atoi(file_number->GetText());
  port_ = atoi(port->GetText());
  io_thread_ = atoi(io_thread->GetText());
  std::cout << "port:" << port_ << ", number of io thread:" << io_thread_
            << std::endl;
}

} // namespace rocket