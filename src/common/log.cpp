#include "log.h"
#include "config.h"
#include "event_loop.h"
#include "timer_event.h"
#include "util.h"
#include <bits/types/struct_timeval.h>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <thread>

namespace rocket {
static std::unique_ptr<Logger> g_logger = nullptr;

Logger *Logger::GetGlobalLogger() { return g_logger.get(); }
void Logger::InitGlobalLogger() {
  g_logger.reset(new Logger());
  g_logger->Init();
}

Logger::Logger(){};

void Logger::Init() {
  auto config = Config::GetGlobalConfig();
  log_level_ = StringToLogLevel(config->log_level_);
  rpc_sync_logger_ = std::make_unique<SyncLogger>(
      config->file_name_, config->file_path_, std::string("rpc"),
      config->file_max_line_, config->file_number_);
  app_sync_logger_ = std::make_unique<SyncLogger>(
      config->file_name_, config->file_path_, std::string("app"),
      config->file_max_line_, config->file_number_);

  time_event_ = std::make_shared<TimerEvent>(config->interval_, true,
                                             std::bind(&Logger::Async, this));
  EventLoop::GetCurrentEventLoop()->AddTimerEvent(time_event_);
}
void Logger::PushRpcLog(const std::string &msg) {
  rpc_latch.lock();
  rpc_buffer_.push_back(msg);
  rpc_latch.unlock();
}

void Logger::PushAppLog(const std::string &msg) {
  app_latch.lock();
  app_buffer_.push_back(msg);
  app_latch.unlock();
}

void Logger::Stop() {
  EventLoop::GetCurrentEventLoop()->DeleteTimerEvent(time_event_);
  PushRpcLog("Logger Stop\n");
  PushAppLog("Logger Stop\n");
  Async();
  rpc_sync_logger_->Stop();
  app_sync_logger_->Stop();
}

void Logger::Async() {
  std::vector<std::string> tmp_app;
  std::vector<std::string> tmp_rpc;

  app_latch.lock();
  tmp_app.swap(app_buffer_);
  app_latch.unlock();

  rpc_latch.lock();
  tmp_rpc.swap(rpc_buffer_);
  rpc_latch.unlock();

  rpc_sync_logger_->PushLog(tmp_rpc);
  app_sync_logger_->PushLog(tmp_app);
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

SyncLogger::SyncLogger(const std::string &file_name,
                       const std::string &file_path,
                       const std::string &log_type,
                       const uint32_t file_max_line, const uint32_t file_number)
    : file_name_(file_name), file_path_(file_path), log_type_(log_type),
      file_max_line_(file_max_line), file_number_(file_number) {

  sem_init(&sem_, 0, 0);
  thread_ = std::thread(std::bind(&SyncLogger::Write, this));
}

SyncLogger::~SyncLogger() {
  Stop();
  sem_destroy(&sem_);
}
void SyncLogger::PushLog(std::vector<std::string> &v) {
  latch_.lock();
  queue_buffer_.push(v);
  latch_.unlock();
  sem_post(&sem_);
}

void SyncLogger::Stop() {
  // 停止前先写完剩余日志
  is_stop_ = true;
  // 唤醒线程
  sem_post(&sem_);
  if (thread_.joinable()) {
    thread_.join();
  }
  if (ofs_.is_open()) {
    ofs_.close();
  }
}

void SyncLogger::Write() {
  std::stringstream ss;
  auto open_file = [&] {
    std::stringstream s;
    s << std::put_time(&time_, "%Y-%m-%d") << ":"
      << std::to_string(file_number_);
    ofs_.open(ss.str() + s.str(), std::ios::app);
    if (!ofs_.is_open()) {
      std::cerr << "log file can not open!" << std::endl;
      exit(1);
    }
  };
  ss << file_path_ << log_type_;
  std::filesystem::create_directories(ss.str());
  ss << '/' << file_name_ << ':';

  while (true) {

    sem_wait(&sem_);
    std::vector<std::string> tmp;
    latch_.lock();
    if (is_stop_ && queue_buffer_.empty()) {
      latch_.unlock();
      return;
    }
    tmp = queue_buffer_.front();
    queue_buffer_.pop();
    latch_.unlock();

    // 获取时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm *now_tm = std::localtime(&now_time);
    if (now_tm->tm_year != time_.tm_year || now_tm->tm_mon != time_.tm_mon ||
        now_tm->tm_mday != time_.tm_mday) {
      time_ = *now_tm;
      ofs_.close();
      num_ = 0;
      ++file_number_;
    }

    if (!ofs_.is_open()) {
      open_file();
    }
    for (auto &i : tmp) {
      ofs_ << i;
      ++num_;
      if (num_ == file_max_line_) {
        ofs_.flush();
        ofs_.close();
        num_ = 0;
        ++file_number_;
        open_file();
      }
    }
    ofs_.flush();
  }
}

std::string formatString(const char *format) { return format; }
} // namespace rocket
