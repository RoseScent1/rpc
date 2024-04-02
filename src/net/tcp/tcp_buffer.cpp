#include "tcp_buffer.h"
#include "log.h"
#include <cstring>

namespace rocket {
TcpBuffer::TcpBuffer(int size)  :read_index_(0), write_index_(0),size_(size) {
  buffer_.resize(size);
}

TcpBuffer::~TcpBuffer() {}

int TcpBuffer::ReadAble() { return write_index_ - read_index_; }

int TcpBuffer::WriteAble() { return size_ - write_index_; }

int TcpBuffer::ReadIndex() { return read_index_; }

int TcpBuffer::WriteIndex() { return write_index_; }

void TcpBuffer::WriteToBuffer(const char *s, int size) {
  if (size > WriteAble()) {
    // 调整buffer大小
    Resize((size + write_index_) * 1.5);
  }
  memcpy(&buffer_[write_index_], s, size);
  write_index_ += size;
}

void TcpBuffer::ReadFromBuffer(std::string &s, int size) {
  if (ReadAble() == 0) {
    return;
  }
  s.resize(size);
  size = std::min(ReadAble(), size);
  memcpy(&s[0], &buffer_[read_index_], size);
  read_index_ += size;
  AdjustBuffer();
}

void TcpBuffer::Resize(int size) {
  buffer_.erase(0, read_index_);
  buffer_.resize(size);
  int count = ReadAble();
  read_index_ = 0;
  read_index_ = std::min(size, count);
  size_ = size;
}

void TcpBuffer::AdjustBuffer() {
  if (read_index_ < size_ / 3) {
    return;
  }
  buffer_.erase(0, read_index_);
  buffer_.resize(size_);
  write_index_ -= read_index_;
  read_index_ = 0;
}

void TcpBuffer::ModifyReadIndex(uint32_t size) {
  if (read_index_ + size >= size_) {
    ERRORLOG("move read index error size too big");
    exit(0);
  }
  read_index_ += size;
  AdjustBuffer();
}
void TcpBuffer::ModifyWriteIndex(uint32_t size) {
  if (write_index_ + size >= size_) {
    ERRORLOG("move write index error size too big");
    exit(0);
  }
  write_index_ += size;
}
} // namespace rocket