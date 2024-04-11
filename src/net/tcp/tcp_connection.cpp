#include "tcp_connection.h"
#include "abstract_coder.h"
#include "abstract_protocol.h"
#include "event_loop.h"
#include "fd_event.h"
#include "fd_event_group.h"
#include "log.h"
#include "rpc_dispatcher.h"
#include "tcp_buffer.h"
#include "tinypb_coder.h"
#include "tinypb_protocol.h"
#include "util.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace rocket {

TcpConnection::TcpConnection(EventLoop *event_loop, int fd, int buffer_size,
                             NetAddr::s_ptr client_addr, ConnectionType type)
    : peer_addr_(client_addr), event_loop_(event_loop), state_(NotConnected),
      connection_type(type) {

  coder_ = std::make_shared<TinyPBCoder>();

  in_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  out_buffer_ = std::make_shared<TcpBuffer>(buffer_size);
  fd_event_ = FdEventGroup::GetFdEventGRoup()->GetFdEvent(fd);
  fd_event_->SetNonBlock();
  if (connection_type == TcpConnection::ConnectionType::Server) {
    dispatcher_ = std::make_shared<RpcDispatcher>();
    listenRead();
  }
}
TcpConnection::~TcpConnection() {
  // INFOLOG("~TcpConnection ");
  close(fd_event_->GetFd());
}

void TcpConnection::Setstate(const TcpState state) { state_ = state; }

TcpConnection::TcpState TcpConnection::GetState() const { return state_; }

void TcpConnection::SetType(TcpConnection::ConnectionType type) {
  connection_type = type;
}

void TcpConnection::Read() {
  // 从socket缓冲区调用系统read读取数据到inbuffer
  if (state_ != Connected) {
    ERRORLOG("read disconnected, client addr[%s],client fd[%d]",
             peer_addr_->ToString().c_str(), fd_event_->GetFd());
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
    INFOLOG("success read %d bytes from %s, peer fd = %d", rt,
            peer_addr_->ToString().c_str(), fd_event_->GetFd());

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
    } else {
      event_loop_->DeleteEpollEvent(fd_event_.get());
      state_ = TcpConnection::NotConnected;
      return;
    }
  }

  // 处理关闭连接
  if (is_close) {
    DEBUGLOG("client closed, client addr = %s, client_fd = %d",
             peer_addr_->ToString().c_str(), fd_event_->GetFd());
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
  std::vector<AbstractProtocol::s_ptr> result;

  if (connection_type == ConnectionType::Server) {
    std::vector<AbstractProtocol::s_ptr> replay_message;
    // 1. 针对每一个请求,调用rpc方法,获取返回message
    // 2. 将返回message放入发送缓冲区,监听可写事件

    coder_->Decode(result, in_buffer_);

    for (auto &i : result) {
      INFOLOG("success get request[%d] from client[%s]", i->msg_id_,
              peer_addr_->ToString().c_str());
      auto message = std::make_shared<TinyPBProtocol>();

      // TODO:有问题
      RpcDispatcher::GetRpcDispatcher()->Dispatch(i, message);

      replay_message.emplace_back(message);
    }
    coder_->EnCode(replay_message, out_buffer_);
    ListenWrite();
  } else {
    // 从buffer解码得到message对象
    coder_->Decode(result, in_buffer_);
    for (auto &i : result) {
      auto it = read_done_.find(i->msg_id_);
      if (it != read_done_.end()) {
        INFOLOG("msg_id = [%d]", i->msg_id_);
        it->second(i);
      }
    }
  }
}

void TcpConnection::Write() {
  if (state_ != Connected) {
    ERRORLOG("write disconnected, client addr[%s],client fd[%d]",
             peer_addr_->ToString().c_str(), fd_event_->GetFd());
    return;
  }

  if (connection_type == TcpConnection::ConnectionType::Client) {
    // 将message序列化
    std::vector<AbstractProtocol::s_ptr> message;
    for (auto &[protocol, done] : write_done_) {
      message.emplace_back(protocol);
    }
    // 将数据写入buffer，然后全部发送

    coder_->EnCode(message, out_buffer_);
  }
  bool is_write_all{false};
  while (!is_write_all) {
    int write_size = out_buffer_->ReadAble();
    if (write_size == 0) {
      DEBUGLOG("no data need to send to client [%s]",
               peer_addr_->ToString().c_str());
      is_write_all = true;
      break;
    }
    int read_index = out_buffer_->ReadIndex();
    int rt = write(fd_event_->GetFd(), &out_buffer_->buffer_[read_index],
                   write_size);
    if (rt >= write_size) {
      DEBUGLOG("write success %d bytes to client [%s]", rt,
               peer_addr_->ToString().c_str());
      is_write_all = true;
      out_buffer_->ModifyReadIndex(rt);
      break;
    }
    if (rt == -1 && errno == EAGAIN) {
      // 发送缓冲区满
      ERRORLOG("write data error, errno = EAGAIN and rt == -1");
      break;
    } else if (rt == -1) {
      event_loop_->DeleteEpollEvent(fd_event_.get());
      state_ = TcpConnection::NotConnected;
      break;
    }
    out_buffer_->ModifyReadIndex(rt);
  }
  if (is_write_all) {
    fd_event_->Cancel(FdEvent::OUT_EVENT);
    event_loop_->AddEpollEvent(fd_event_.get());
  }

  if (connection_type == TcpConnection::Client &&
      state_ == TcpConnection::Connected) {
    for (auto &[arg, func] : write_done_) {
      func(arg);
    }
    write_done_.clear();
  }
}

void TcpConnection::Clear() {
  // 服务器处理关闭连接后的清理动作
  if (state_ == NotConnected) {
    return;
  }

  event_loop_->DeleteEpollEvent(fd_event_.get());
  state_ = NotConnected;
}

void TcpConnection::ShutDown() {
  if (state_ == NotConnected) {
    return;
  }
  state_ = HalfClosing;
  shutdown(fd_event_->GetFd(), SHUT_RDWR);
}

void TcpConnection::ListenWrite() {
  fd_event_->Listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::Write, this));
  event_loop_->AddEpollEvent(fd_event_.get());
}
void TcpConnection::listenRead() {
  fd_event_->Listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::Read, this));
  event_loop_->AddEpollEvent(fd_event_.get());
}

void TcpConnection::PushWriteMessage(
    AbstractProtocol::s_ptr &arg,
    std::function<void(AbstractProtocol::s_ptr)> &func) {
  write_done_.emplace_back(arg, func);
}

void TcpConnection::PushReadMessage(
    const uint32_t msg_id, std::function<void(AbstractProtocol::s_ptr)> &func) {
  read_done_.insert({msg_id, func});
}

} // namespace rocket