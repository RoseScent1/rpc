#pragma once
#include "net_addr.h"
#include <memory>
#include <utility>
namespace rocket {
class TcpAcceptor {
public:
	using s_ptr = std::shared_ptr<TcpAcceptor>;
	TcpAcceptor(NetAddr::s_ptr addr);
	~TcpAcceptor();

	std::pair<int,NetAddr::s_ptr> Accept();

	int GetListenFd();
private:
// 本地ip:port addr;
// listenfd
	NetAddr::s_ptr addr_;
	int listen_fd_;
	int family_;
};
} // namespace rocket