#include "config.h"
#include "log.h"
#include <thread>
#include <vector>
int main() {
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");
  rocket::Logger::InitGlobalLogger();
  std::vector<std::thread> thread;
  for (int i = 1; i <= 100; ++i) {
    thread.emplace_back([]() { DEBUGLOG("test log %s", "debug"); });
    thread.emplace_back([]() { INFOLOG("test log %s", "info"); });
    thread.emplace_back([]() { ERRORLOG("test log %s", "error"); });
  }
  for (auto &&i : thread) {
    i.join();
  }
}