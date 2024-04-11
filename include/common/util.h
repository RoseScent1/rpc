#pragma once

#include <cstdint>
#include <sched.h>
#include <string>
namespace rocket {
pid_t getPid();
pid_t getThreadid();
int64_t getNowMs();
uint32_t getInt32FromNetByte(const char * buf);
uint32_t GenMsgId();
} // namespace rocket