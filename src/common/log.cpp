#include "log.h"
#include "config.h"
#include "util.h"
#include <bits/types/struct_timeval.h>
#include <cstddef>
#include <ctime>
#include <mutex>
#include <sstream>
#include <string>
#include <sys/time.h>

namespace rocket {
static Logger *g_logger = nullptr;

Logger *Logger::GetGlobalLogger() { return g_logger; }
void Logger::InitGlobalLogger() {
  g_logger =
      new Logger(StringToLogLevel(Config::GetGlobalConfig()->log_level_));
}
void Logger::PushLog(const std::string &msg) {
  std::unique_lock<std::mutex> lock(latch_);
  buffer_.push(msg);
}

void Logger::Log() {
  std::unique_lock<std::mutex> lock(latch_);
	std::queue<std::string> tmp;
	tmp.swap(buffer_);
	lock.unlock();
  while (!tmp.empty()) {
    std::cout << tmp.front();
    tmp.pop();
  }
}

std::string LogLevelToString(LogLevel log_level) {
  switch (log_level) {
  case Debug:
    return "Debug";
  case Info:
    return "Info";
  case Error:
    return "Error";
  default:
    return "Unknown";
  }
}
auto StringToLogLevel(const std::string &log_level) -> LogLevel {
  if (log_level == "debug") {
    return LogLevel::Debug;
  }
  if (log_level == "Info") {
    return LogLevel::Info;
  }
  if (log_level == "Error") {
    return LogLevel::Error;
  }
  return LogLevel::Unknow;
}
std::string LogEvent::ToString() {
  timeval now_time;

  gettimeofday(&now_time, nullptr);

  tm now_time_t;
  localtime_r(&(now_time.tv_sec), &now_time_t);

  char buf[128];
  strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);

  std::string time_str(buf);
  int ms = now_time.tv_usec / 1000;
  time_str = time_str + "." + std::to_string(ms);

  thread_id_ = getThreadid();
  pid_ = getPid();

  std::stringstream ss;
  ss << "[" << LogLevelToString(level_) << "]\t[" << time_str << "]\t["
     << std::to_string(pid_) << ":" << std::to_string(thread_id_) << "]\t";
  return ss.str();
}

} // namespace rocket
