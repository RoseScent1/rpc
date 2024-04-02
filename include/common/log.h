#pragma once
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
namespace rocket {
// 格式化字符串
template <typename... Args>
std::string formatString(const char* format, Args&&... args) {
    auto size = snprintf(nullptr, 0, format, std::forward<Args>(args)...);

    std::string result;
    if (size > 0) {
        result.resize(size);
        snprintf(&result[0], size + 1, format, std::forward<Args>(args)...);
    }

    return result;
}
//  打印log宏
#define DEBUGLOG(str, ...)                                                     \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Debug) {                                               \
    std::string msg =                                                          \
        (new rocket::LogEvent(rocket::LogLevel::Debug))->ToString() + "[" +    \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    msg += "\n";                                                               \
    rocket::Logger::GetGlobalLogger()->PushLog(msg);                           \
    rocket::Logger::GetGlobalLogger()->Log();                                  \
  }

#define INFOLOG(str, ...)                                                      \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Info) {                                                \
    std::string msg =                                                          \
        (new rocket::LogEvent(rocket::LogLevel::Info))->ToString() + "[" +     \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    msg += "\n";                                                               \
    rocket::Logger::GetGlobalLogger()->PushLog(msg);                           \
    rocket::Logger::GetGlobalLogger()->Log();                                  \
  }

#define ERRORLOG(str, ...)                                                     \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Error) {                                               \
    std::string msg =                                                          \
        (new rocket::LogEvent(rocket::LogLevel::Error))->ToString() + "[" +    \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    msg += "\n";                                                               \
    rocket::Logger::GetGlobalLogger()->PushLog(msg);                           \
    rocket::Logger::GetGlobalLogger()->Log();                                  \
  }

// 日志级别
enum LogLevel { Unknow = 0, Debug, Info, Error };

// 打印日志
class Logger {
public:
  using s_ptr = std::shared_ptr<Logger>;

  Logger(LogLevel level) : set_level_(level){};

  void PushLog(const std::string &msg);
  LogLevel GetLogLevel() { return set_level_; }
  void Log();
  static void InitGlobalLogger();
  static Logger *GetGlobalLogger();

private:
  LogLevel set_level_;
  std::mutex latch_;
  std::queue<std::string> buffer_;
};

auto LogLevelToString(LogLevel log_level) -> std::string;
auto StringToLogLevel(const std::string &log_level) -> LogLevel;
// 日志事件
class LogEvent {
public:
  LogEvent(LogLevel level) : level_(level){};
  auto GetFileName() -> std::string { return file_name_; }
  auto GetLogLevel() -> LogLevel { return level_; }

  auto ToString() -> std::string;

private:
  // 文件吗
  std::string file_name_;
  // 行号
  // int32_t file_line_;
  // 进程号,线程号,日志级别
  int32_t pid_;
  int32_t thread_id_;
  LogLevel level_;
};

} // namespace rocket
