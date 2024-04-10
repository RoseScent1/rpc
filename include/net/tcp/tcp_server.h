#pragma once
#include "event_loop.h"
#include "fd_event.h"
#include "io_thread_group.h"
#include "net_addr.h"
#include "tcp_acceptor.h"
#include "tcp_connection.h"
#include <set>
namespace rocket {

class TcpServer {
public:
  TcpServer(NetAddr::s_ptr addr);
  ~TcpServer();
  void start();


private:
  // 新客户端连接后执行的方法
  void OnAccept();
  void init();
  TcpAcceptor::s_ptr acceptor_;

  NetAddr::s_ptr addr_; // 本地监听地址

  EventLoop *main_event_loop_; // 主Reactor

  IOThreadGroup *io_thread_group_; // subReactor

  FdEvent::s_ptr listen_fd_event_;

	int client_count_;
	std::set<TcpConnection::s_ptr> connections_;
};
} // namespace rocket