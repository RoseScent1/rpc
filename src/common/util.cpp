#include "util.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
namespace rocket {
static int g_pid = 0;
static thread_local int g_thread_id = 0;
pid_t getPid() { return g_pid == 0 ? getpid() : g_pid; }
pid_t getThreadid() {
  return g_thread_id == 0 ? syscall(SYS_gettid) : g_thread_id;
}
int64_t getNowMs() {
  timeval now_time;
  gettimeofday(&now_time, nullptr);
  return now_time.tv_sec * 1000 + now_time.tv_usec / 1000;
}

uint32_t getInt32FromNetByte(const char *buf) {
  uint32_t res;
  memcpy(&res, buf, sizeof(res));
  return ntohl(res);
}

static thread_local uint32_t msg_id = 0;
uint32_t GenMsgId() { return msg_id++; }
}