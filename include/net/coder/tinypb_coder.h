#pragma once

#include "abstract_coder.h"
#include "tinypb_protocol.h"
namespace rocket {
class TinyPBCoder : public AbstractCoder {
public:
  virtual void EnCode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &out_buffer) ;
  virtual void Decode(std::vector<AbstractProtocol::s_ptr> &message,
                      TcpBuffer::s_ptr &out_buffer);
	~TinyPBCoder();
private:
	const char* encodeTinyPB(TinyPBProtocol::s_ptr message, int &len);
};
}