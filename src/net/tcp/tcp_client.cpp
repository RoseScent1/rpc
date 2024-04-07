#include "tcp_client.h"
#include "event_loop.h"
#include "fd_event.h"
#include "fd_event_group.h"
#include "log.h"
#include "tcp_connection.h"
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
namespace rocket {
TcpClient::TcpClient(NetAddr::s_ptr ser_addr) : ser_addr_(ser_addr) {
  event_loop_ = EventLoop::GetCurrentEventLoop();
  fd_ = socket(ser_addr->GetFamily(), SOCK_STREAM, 0);
  if (fd_ == -1) {
    ERRORLOG("TcpClient::TcpClient() error,failed to create fd");
    return;
  }
  fd_event_ = FdEventGroup::GetFdEventGRoup()->GetFdEvent(fd_);
  fd_event_->SetNonBlock();
  connection_ = std::make_shared<TcpConnection>(
      event_loop_, fd_, 128, ser_addr, TcpConnection::ConnectionType::Client);
  connection_->SetType(TcpConnection::ConnectionType::Client);
}
TcpClient::~TcpClient() {
  if (fd_ > 0) {
    close(fd_);
  }
}

// 异步进行connect
// 调用成功,done 执行
void TcpClient::Connect(std::function<void()> done) {
  int rt = connect(fd_, ser_addr_->GetSockAddr(), ser_addr_->GetSockLen());
  if (rt == 0) {
    INFOLOG("connect addr[%s] success", ser_addr_->ToString().c_str());
    if (done) {
      connection_->Setstate(TcpConnection::Connected);
      done();
    }
    event_loop_->Loop();
  } else if (rt == -1) {
    if (errno == EINPROGRESS) {
      // epoll监听可写事件,判断错误码
      fd_event_->Listen(FdEvent::OUT_EVENT, [this, done]() {
        bool is_connect{false};
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);
        if (error == 0) {
          INFOLOG("connect addr[%s] success", ser_addr_->ToString().c_str());
          is_connect = true;
        } else {
          ERRORLOG("connect error,errno = %d, error = %s", errno,
                   strerror(errno));
        }
        fd_event_->Cancel(FdEvent::OUT_EVENT);
        event_loop_->AddEpollEvent(fd_event_.get());

        // 连接成功才会执行
        if (is_connect && done) {
          connection_->Setstate(TcpConnection::Connected);
          done();
        }
      });
      event_loop_->AddEpollEvent(fd_event_.get());
      event_loop_->Loop();
    } else {
      ERRORLOG("connect error,errno = %d, error = %s", errno, strerror(errno));
    }
  }
}

// 异步发送message
// 发送message成功调用done,入参就是message对象
void TcpClient::WriteMessage(
    AbstractProtocol::s_ptr message,
    std::function<void(AbstractProtocol::s_ptr)> done) {
  // 1. 把message对象写入到connenct的buffer,done写入
  // 2. 启功connection可写事件
  connection_->PushWriteMessage(message, done);
  connection_->ListenWrite();
}

// 异步读取message
// 读取message成功调用done,入参就是message对象
void TcpClient::ReadMessage(
    const std::string &req_id, std::function<void(AbstractProtocol::s_ptr)>
        done) {

  // 1. 监听可写
  // 2. 从buffer里decode对象,判断req_id是否相等，相等则读成功，执行回调函数
	connection_->PushReadMessage(req_id, done);
	connection_->listenRead();
}

} // namespace rocket