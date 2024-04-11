#pragma once
#include "timer_event.h"
#include <bits/types/struct_tm.h>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <semaphore.h>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
namespace rocket {
// 解决snprintf警告,只允许类型未整型,或者 字符串传入
template <typename T> struct is_char : std::false_type {};
template <> struct is_char<char *> : std::true_type {};
template <> struct is_char<const char *> : std::true_type {};
template <typename T> constexpr bool is_char_v = is_char<T>::value;

template <typename T, typename... Args>
struct is_ok : std::conditional_t<(is_char_v<std::remove_reference_t<T>> ||
                                   std::is_integral_v<std::remove_reference_t<
                                       T>>)&&is_ok<Args...>::value,
                                  std::true_type, std::false_type> {};
template <typename T>
struct is_ok<T>
    : std::conditional_t<is_char_v<std::remove_reference_t<T>> ||
                             std::is_integral_v<std::remove_reference_t<T>>,
                         std::true_type, std::false_type> {};
template <bool> struct ret_type { using type = std::string; };
template <> struct ret_type<false> {};
// 格式化字符串

template <typename... Args>
typename ret_type<is_ok<Args...>::value>::type formatString(const char *format,
                                                            Args &&...args) {
  auto size = snprintf(nullptr, 0, format, std::forward<Args>(args)...);

  std::string result;
  if (size > 0) {
    result.resize(size);
    snprintf(&result[0], size + 1, format, std::forward<Args>(args)...);
  }

  return result;
}
std::string formatString(const char *format);

//  打印log宏
#define RPC_DEBUG_LOG(str, ...)                                                \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Debug) {                                               \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Debug)).ToString() + "[" +         \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushRpcLog(log_msg);                    \
  }

#define RPC_INFO_LOG(str, ...)                                                 \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Info) {                                                \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Info)).ToString() + "[" +          \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushRpcLog(log_msg);                    \
  }

#define RPC_ERROR_LOG(str, ...)                                                \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Error) {                                               \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Error)).ToString() + "[" +         \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushRpcLog(log_msg);                    \
  }

#define APP_DEBUG_LOG(str, ...)                                                \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Debug) {                                               \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Debug)).ToString() + "[" +         \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushAppLog(log_msg);                    \
  }

#define APP_INFO_LOG(str, ...)                                                 \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Info) {                                                \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Info)).ToString() + "[" +          \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushAppLog(log_msg);                    \
  }

#define APP_ERROR_LOG(str, ...)                                                \
  if (rocket::Logger::GetGlobalLogger()->GetLogLevel() <=                      \
      rocket::LogLevel::Error) {                                               \
    std::string log_msg =                                                      \
        (rocket::LogEvent(rocket::LogLevel::Error)).ToString() + "[" +         \
        std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" +       \
        rocket::formatString(str, ##__VA_ARGS__);                              \
    log_msg += "\n";                                                           \
    rocket::Logger::GetGlobalLogger()->PushAppLog(log_msg);                    \
  }

class SyncLogger {
public:
  SyncLogger(const std::string &file_name, const std::string &file_path,
             const std::string &log_type, const uint32_t file_max_line,
             uint32_t file_num);
  ~SyncLogger();

  void Stop();

  void PushLog(std::vector<std::string> &v);
  // 负责写的函数
  void Write();

private:
  //日志写入文件的信息
  std::string file_name_;
  std::string file_path_;
  std::string log_type_; // 程序底层和应用层,两种

  uint32_t file_max_line_; //最大写入行数
  uint32_t num_{0};        //当前写入了多少行

  uint32_t file_number_{0}; // 文件编号
  tm time_{};               // 存储时间,判断是否需要创建新文件

  sem_t sem_; // 控制线程的运行,间隔时间到了才唤醒

  std::queue<std::vector<std::string>> queue_buffer_;
  std::mutex latch_; // queue 锁

  std::thread thread_; //负责写 的线程;

  std::ofstream ofs_; //当前写入的文件

  bool is_stop_{false};
};

// 日志级别
enum LogLevel { Unknow = 0, Debug, Info, Error };

// 打印日志
class Logger {
public:
  using s_ptr = std::shared_ptr<Logger>;

  Logger();

  void Init();
  void PushRpcLog(const std::string &msg);
  void PushAppLog(const std::string &msg);
	void Stop();
  LogLevel GetLogLevel() { return log_level_; }
  void Async();
  static void InitGlobalLogger();
  static Logger *GetGlobalLogger();

private:
  LogLevel log_level_;
  std::mutex rpc_latch;
  std::vector<std::string> rpc_buffer_;
  std::mutex app_latch;
  std::vector<std::string> app_buffer_;
  std::unique_ptr<SyncLogger> rpc_sync_logger_;
  std::unique_ptr<SyncLogger> app_sync_logger_;
  TimerEvent::s_ptr time_event_;
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
  // 文件
  std::string file_name_;
  // 行号
  // int32_t file_line_;
  // 进程号,线程号,日志级别
  int32_t pid_;
  int32_t thread_id_;
  LogLevel level_;
};

} // namespace rocket
