#pragma once

#include "event_loop.h"
#include "fd_event.h"
#include "io_thread.h"
#include "net_addr.h"
#include "tcp_buffer.h"
#include <memory>
namespace rocket {
class TcpConnection {
public:
	using s_ptr = std::shared_ptr<TcpConnection>;
	enum ConnectionType {
		Server = 1,
		Client = 2
	};
  enum TcpState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4
  };

  TcpConnection(EventLoop *event_loop, int fd, int buffer_size,
                NetAddr::s_ptr client_addr);
  ~TcpConnection();
  void Read();
  void Execute();
  void Write();
	void Setstate(const TcpState state);
	TcpState GetState() const ;
	void Clear();
	void ShutDown();
	void SetType(TcpConnection::ConnectionType type) ;
private:
  IPNetAddr::s_ptr local_addr_;
  IPNetAddr::s_ptr client_addr_;
  TcpBuffer::s_ptr in_buffer_;
  TcpBuffer::s_ptr out_buffer_;

	EventLoop *event_loop_; // 当前连接所属IO线程

  FdEvent::s_ptr fd_event_;

	TcpState state_;
	ConnectionType connection_type;
};
} // namespace rocket