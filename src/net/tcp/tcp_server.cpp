#include "tcp_server.h"
#include "event_loop.h"
#include "fd_event.h"
#include "io_thread_group.h"
#include "log.h"
#include "tcp_acceptor.h"
#include <functional>

namespace rocket {

TcpServer::TcpServer(NetAddr::s_ptr addr) : addr_(addr), client_count_(0) {
  init();
  INFOLOG("rocket RPC tcpServer listen sucess on %s", addr->ToString().c_str());
}

TcpServer::~TcpServer() {
  if (main_event_loop_) {
    delete main_event_loop_;
    main_event_loop_ = nullptr;
  }
  if (io_thread_group_) {
    delete io_thread_group_;
    io_thread_group_ = nullptr;
  }
  if (listen_fd_event_) {
    delete listen_fd_event_;
    io_thread_group_ = nullptr;
  }
}

void TcpServer::OnAccept() {
  int client_fd = acceptor_->Accept();

  ++client_count_;
  // TODO: 把client_fd添加到IO线程
  // io_thread_group_->GetIOThread()->GetEventloop()->AddEpollEvent(FdEvent
  // *event);
  INFOLOG("TcpServer success get client, fd=%d", client_fd);
}

void TcpServer::init() {

  acceptor_ = std::make_shared<TcpAcceptor>(addr_);

  main_event_loop_ = EventLoop::GetCurrentEventLoop();

  io_thread_group_ = new IOThreadGroup(4);
  listen_fd_event_ = new FdEvent(acceptor_->GetListenFd());
  listen_fd_event_->Listen(FdEvent::IN_EVENT,
                           std::bind(&TcpServer::OnAccept, this));
  main_event_loop_->AddEpollEvent(listen_fd_event_);
}

void TcpServer::start() {
  io_thread_group_->Start();
  main_event_loop_->Loop();
}
} // namespace rocket