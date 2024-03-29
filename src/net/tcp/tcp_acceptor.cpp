#include "tcp_acceptor.h"
#include "log.h"
#include "net_addr.h"
#include <asm-generic/socket.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
namespace rocket {
TcpAcceptor::TcpAcceptor(NetAddr::s_ptr addr) : addr_(addr) {
  if (!addr->CheckValid()) {
    ERRORLOG("addr is invalid %s", addr_->ToString().c_str());
    exit(0);
  }
  family_ = addr->GetFamily();

  listen_fd_ = socket(family_, SOCK_STREAM, 0);
  if (listen_fd_ < 0) {
    ERRORLOG("listen_fd creat false   fd = %d", listen_fd_);
    exit(0);
  }

  int val = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) !=
      0) {
    ERRORLOG("set sockopt REUSEADDR error errno = %d, %s", errno,
             strerror(errno));
  }

  socklen_t len = addr_->GetSockLen();
  if (bind(listen_fd_, addr_->GetSockAddr(), len) != 0) {
    ERRORLOG("bind error errno %d, %s", errno, strerror(errno));
    exit(0);
  }

  if (listen(listen_fd_, 1000) != 0) {
    ERRORLOG("listen error errno %d, %s", errno, strerror(errno));
    exit(0);
  }
}
TcpAcceptor::~TcpAcceptor() {}

int TcpAcceptor::Accept() {
  if (family_ == AF_INET) {
    sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t len = sizeof(client_addr);
    int client_fd =
        accept(listen_fd_, reinterpret_cast<sockaddr *>(&client_addr), &len);
    if (client_fd < 0) {
      ERRORLOG("accept error errno %d, %s", errno, strerror(errno));
    }
		INFOLOG("A client have accepted ,client addr=%s",IPNetAddr(client_addr).ToString().c_str());
		return client_fd;
  } else {
    //...
  }
}

} // namespace rocket