#include "tcp_connection.h"
#include "fd_event.h"
#include "fd_event_group.h"
#include "log.h"
#include "tcp_buffer.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>

namespace rocket {

TcpConnection::TcpConnection(IOThread *io_thread, int fd, int buffer_size,
                             NetAddr::s_ptr client_addr)
    : client_addr_(client_addr), io_thread_(io_thread), state_(NotConnected) {

  in_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  out_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  fd_event_ = FdEventGroup::GetFdEventGRoup()->GetFdEvent(fd);
  fd_event_->SetNonBlock();

  fd_event_->Listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::Read, this));
	io_thread_->GetEventloop()->AddEpollEvent(fd_event_.get());

}
TcpConnection::~TcpConnection() {
	DEBUGLOG("Tcpconnection destory");
}

void TcpConnection::Setstate(const TcpState state) { state_ = state; }

TcpConnection::TcpState TcpConnection::GetState() const { return state_; }

void TcpConnection::Read() {
  // 从socket缓冲区调用系统read读取数据到inbuffer
  if (state_ != Connected) {
    ERRORLOG("read disconnected, client addr[%s],client fd[%d]",
             client_addr_->ToString().c_str(), fd_event_->GetFd());
    return;
  }
  // 是否读完？
  bool is_read_all{false};
  bool is_close{false};

  while (!is_read_all) {
    //
    if (in_buffer_->WriteAble() == 0) {
      in_buffer_->Resize(2 * in_buffer_->buffer_.size());
    }
    int read_count = in_buffer_->WriteAble();
    int write_index = in_buffer_->WriteIndex();

    int rt = read(fd_event_->GetFd(), &(in_buffer_->buffer_[write_index]),
                  read_count);

    INFOLOG("success read %d bytes fron %s, client fd = %d", rt,
            client_addr_->ToString().c_str(), fd_event_->GetFd());

    // 读成功了！进行调整
    if (rt > 0) {
      in_buffer_->ModifyWriteIndex(rt);

      // 还有数据没有读完
      if (rt == read_count) {
        continue;
      } else if (rt < read_count) {
        is_read_all = true;
        break;
      }

    } else if (rt == 0) {
      is_close = true;
      break;
    } else if (rt == -1 && errno == EAGAIN) {
      is_read_all = true;
      break;
    }
  }

  // 处理关闭连接
  if (is_close) {
    DEBUGLOG("peer closed, peer addr = %s, client_fd = %d",
             client_addr_->ToString().c_str(), fd_event_->GetFd());
    Clear();
    return;
  }

  if (!is_read_all) {
    INFOLOG("not read all data");
  }
  // 读完就开始进行rpc解析
  Execute();
}

void TcpConnection::Execute() {
  // 将RPC请求执行业务逻辑, 获取RPC响应, 再把RPC响应发送回去
  std::string buffer;
  in_buffer_->ReadFromBuffer(buffer, in_buffer_->ReadAble());

  INFOLOG("sucess get request from client[%s],out buffer =%s",
          client_addr_->ToString().c_str(),buffer.c_str());
  out_buffer_->WriteToBuffer(buffer.c_str(), buffer.size());
  fd_event_->Listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::Write, this));
	io_thread_->GetEventloop()->AddEpollEvent(fd_event_.get());
}

void TcpConnection::Write() {
  if (state_ != Connected) {
    ERRORLOG("write disconnected, client addr[%s],client fd[%d]",
             client_addr_->ToString().c_str(), fd_event_->GetFd());
    return;
  }

  bool is_write_all{false};
  while (!is_write_all) {
		std::cout << out_buffer_->buffer_ << std::endl;
    int write_size = out_buffer_->ReadAble();
    if (write_size == 0) {
      DEBUGLOG("no data need to send to client [%s]",
               client_addr_->ToString().c_str());
			is_write_all = true;
      break;
    }
    int read_index = out_buffer_->ReadIndex();
    int rt = write(fd_event_->GetFd(), &out_buffer_->buffer_[read_index],
                   write_size);
    if (rt >= write_size) {
      DEBUGLOG("write success %d bytes client [%s]",rt,
               client_addr_->ToString().c_str());
			is_write_all = true;
      break;
    }
    if (rt == -1 && errno == EAGAIN) {
      // 发送缓冲区满
      ERRORLOG("write data error, errno = EAGAIN and rt == -1");
      break;
    }
    out_buffer_->ModifyReadIndex(rt);
  }
	if (is_write_all) {
		fd_event_->Cancel(FdEvent::OUT_EVENT);
		io_thread_->GetEventloop()->AddEpollEvent(fd_event_.get());
	}
}

void TcpConnection::Clear() {
  // 服务器处理关闭连接后的清理动作
  if (state_ == Closed) {
    return;
  }
	fd_event_->Cancel(FdEvent::IN_EVENT);
	fd_event_->Cancel(FdEvent::OUT_EVENT);
  io_thread_->GetEventloop()->DeleteEpollEvent(fd_event_.get());
  state_ = Closed;
}

void TcpConnection::ShutDown() {
  if (state_ == Closed || state_ == NotConnected) {
    return;
  }
  state_ = HalfClosing;
  shutdown(fd_event_->GetFd(), SHUT_RDWR);
}

} // namespace rocket