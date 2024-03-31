#pragma once
#include "net_addr.h"
#include <memory>
namespace rocket {
class TcpAcceptor {
public:
	using s_ptr = std::shared_ptr<TcpAcceptor>;
	TcpAcceptor(NetAddr::s_ptr addr);
	~TcpAcceptor();

	int Accept();

	int GetListenFd();
private:
// ip:port addr;
// listenfd
	NetAddr::s_ptr addr_;
	int listen_fd_;
	int family_;
};
} // namespace rocket