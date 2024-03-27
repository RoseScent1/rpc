#include "util.h"
#include <sys/syscall.h>
#include <unistd.h>
namespace rocket {
static int g_pid = 0;
static thread_local int g_thread_id = 0;
pid_t getPid() { return g_pid == 0 ? getpid() : g_pid; }
pid_t getThreadid() {
  return g_thread_id == 0 ? syscall(SYS_gettid) : g_thread_id;
}
} // namespace rocket