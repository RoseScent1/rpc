#include "tinypb_protocol.h"
#include "log.h"

namespace rocket {
const char TinyPBProtocol::PB_START = 0x02;
const char TinyPBProtocol::PB_END = 0x03;

TinyPBProtocol::~TinyPBProtocol() {
	// INFOLOG("~TinyPBProtocol");
}
void TinyPBProtocol::SetErrInfo(int32_t err_code, const char *err_info) {
  err_code_ = err_code;
  err_info_ = err_info;
}
} // namespace rocket
