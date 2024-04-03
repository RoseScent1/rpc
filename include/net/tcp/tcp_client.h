#pragma once

#include "abstrack_protocol.h"
#include "event_loop.h"
#include "fd_event.h"
#include "net_addr.h"
#include "tcp_connection.h"
namespace rocket {
class TcpClient {
public:
	TcpClient(NetAddr::s_ptr ser_addr);
	~TcpClient();

	// 异步进行connect
	// 调用成功,done 执行
	void Connect(std::function<void()> done) ;

	// 异步发送message
	// 发送message成功调用done,入参就是message对象
	void WriteMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done );

	// 异步读取message
	// 读取message成功调用done,入参就是message对象
	void ReadMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done );




private:
	NetAddr::s_ptr ser_addr_;
	EventLoop * event_loop_;
	
	int fd_;
	FdEvent::s_ptr fd_event_;
	TcpConnection::s_ptr connection_;
};
}