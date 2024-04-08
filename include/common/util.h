#pragma once

#include <cstdint>
#include <sched.h>
namespace rocket {
pid_t getPid();
pid_t getThreadid();
int64_t getNowMs();
int32_t getInt32FromNetByte(const char * buf);
} // namespace rocket