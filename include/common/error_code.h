#pragma once

#ifndef SYS_ERROR_PREFIX
#define SYS_ERROR_PREFIX(xx) 1000##xx
#endif

constexpr int ERROR_PEER_CLOSE = SYS_ERROR_PREFIX(0);
constexpr int ERROR_FAILED_CONNECT = SYS_ERROR_PREFIX(1);
constexpr int ERROR_FAILED_GET_REPLY = SYS_ERROR_PREFIX(2);
constexpr int ERROR_FAILED_DESERIALIZE = SYS_ERROR_PREFIX(3);
constexpr int ERROR_FAILED_SERIALIZE = SYS_ERROR_PREFIX(4);
constexpr int ERROR_FAILED_ENCODE = SYS_ERROR_PREFIX(5);
constexpr int ERROR_FAILED_DECODE = SYS_ERROR_PREFIX(6);
constexpr int ERROR_RPC_CALL_TIMEOUT = SYS_ERROR_PREFIX(7);
constexpr int ERROR_SERVICE_NOT_FOUND = SYS_ERROR_PREFIX(8);
constexpr int ERROR_METHOD_NOT_FOUND = SYS_ERROR_PREFIX(9);
constexpr int ERROR_PARSE_SERVICE_NAME = SYS_ERROR_PREFIX(10);
