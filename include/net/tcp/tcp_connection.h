#pragma once

#include "abstract_coder.h"
#include "abstract_protocol.h"
#include "event_loop.h"
#include "fd_event.h"
#include "net_addr.h"
#include "rpc_dispatcher.h"
#include "tcp_buffer.h"
#include <map>
#include <memory>
#include <vector>
namespace rocket {
class TcpConnection {
public:
  using s_ptr = std::shared_ptr<TcpConnection>;
  enum ConnectionType { Server = 1, Client = 2 };
  enum TcpState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
  };

  TcpConnection(EventLoop *event_loop, int fd, int buffer_size,
                NetAddr::s_ptr client_addr,
                ConnectionType type = TcpConnection::ConnectionType::Server);
  ~TcpConnection();
  void Read();
  void Execute();
  void Write();
  void Setstate(const TcpState state);
  TcpState GetState() const;
  void Clear();
  void ShutDown();
  void SetType(TcpConnection::ConnectionType type);

  // 启动读写监听
  void ListenWrite();
  void listenRead();

  void PushWriteMessage(AbstractProtocol::s_ptr &arg,
                        std::function<void(AbstractProtocol::s_ptr)> &func);
  void PushReadMessage(const std::string &msg_id,
                   std::function<void(AbstractProtocol::s_ptr)> &func);

private:
  IPNetAddr::s_ptr local_addr_;
  IPNetAddr::s_ptr client_addr_;
  TcpBuffer::s_ptr in_buffer_;
  TcpBuffer::s_ptr out_buffer_;

  EventLoop *event_loop_; // 当前连接所属IO线程

  FdEvent::s_ptr fd_event_;

  TcpState state_;
  ConnectionType connection_type;
  // pair<s_ptr,function>
  std::vector<std::pair<AbstractProtocol::s_ptr,
                        std::function<void(AbstractProtocol::s_ptr)>>>
      write_done_;

  // key msg_id
  std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>>
      read_done_;
  AbstractCoder::s_ptr coder_;

	std::shared_ptr<RpcDispatcher> dispatcher_;
};
} // namespace rocket