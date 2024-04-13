#include "tcp_acceptor.h"
#include "log.h"
#include "net_addr.h"
#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
namespace rocket {
TcpAcceptor::TcpAcceptor(NetAddr::s_ptr addr) : addr_(addr) {
  if (!addr->CheckValid()) {
    RPC_ERROR_LOG("addr is invalid %s", addr_->ToString().c_str());
    exit(6);
  }
  family_ = addr->GetFamily();

  listen_fd_ = socket(family_, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    RPC_ERROR_LOG("listen_fd creat false   fd = %d", listen_fd_);
    exit(8);
  }

  int val = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) !=
      0) {
    RPC_ERROR_LOG("set sockopt REUSEADDR error errno = %d, %s", errno,
                  strerror(errno));
  }

  socklen_t len = addr_->GetSockLen();
  if (bind(listen_fd_, addr_->GetSockAddr(), len) != 0) {
    RPC_ERROR_LOG("bind error errno %d, %s", errno, strerror(errno));
    exit(9);
  }

  if (listen(listen_fd_, 1000) != 0) {
    RPC_ERROR_LOG("listen error errno %d, %s", errno, strerror(errno));
    exit(10);
  }
}
TcpAcceptor::~TcpAcceptor() {
  // std::cout << "析构TcpAcceptor" << std::endl;
  close(listen_fd_);
  RPC_DEBUG_LOG("```````close fd %d `````", listen_fd_);
}

std::pair<int, NetAddr::s_ptr> TcpAcceptor::Accept() {
  if (family_ == AF_INET) {
    sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    int client_fd =
        accept(listen_fd_, reinterpret_cast<sockaddr *>(&client_addr), &len);
    if (client_fd < 0) {
      RPC_ERROR_LOG("accept error errno %d, %s", errno, strerror(errno));
      return {};
    }
    //客户端addr,用于返回值
    IPNetAddr::s_ptr addr = std::make_shared<IPNetAddr>(client_addr);
    RPC_INFO_LOG("A client have accepted ,client addr=%s",
                 IPNetAddr(client_addr).ToString().c_str());
    return {client_fd, addr};
  } else {
    //...
  }
  return {};
}

int TcpAcceptor::GetListenFd() { return listen_fd_; }

} // namespace rocket