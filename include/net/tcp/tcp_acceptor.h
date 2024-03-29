#pragma once
#include "net_addr.h"
namespace rocket {
class TcpAcceptor {
public:
	TcpAcceptor(NetAddr::s_ptr addr);
	~TcpAcceptor();

	int Accept();

private:
// ip:port addr;
// listenfd
	NetAddr::s_ptr addr_;
	int listen_fd_;
	int family_;
};
} // namespace rocket