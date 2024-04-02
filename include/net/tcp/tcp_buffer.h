#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
namespace rocket {
class TcpBuffer {
public:
  using s_ptr = std::shared_ptr<TcpBuffer>;

  TcpBuffer(int size);
  ~TcpBuffer();

  int ReadAble();
  int WriteAble();
  int ReadIndex();
  int WriteIndex();

  void WriteToBuffer(const char *s, int size);
  void ReadFromBuffer(std::string &s, int size);

  void Resize(int size);
  void AdjustBuffer();

  void ModifyReadIndex(uint32_t size);
  void ModifyWriteIndex(uint32_t size);

private:
  int read_index_;
  int write_index_;
  int size_;

public:
  std::string buffer_;
};
} // namespace rocket