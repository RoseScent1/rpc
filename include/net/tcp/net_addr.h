#pragma once

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
namespace rocket {
class NetAddr {
public:
	using s_ptr = std::shared_ptr<NetAddr>;
  virtual sockaddr *GetSockAddr() = 0;
  virtual socklen_t GetSockLen() = 0;
  virtual int GetFamily() = 0;

  virtual std::string ToString() = 0;
	
	virtual bool CheckValid() = 0;
private:
};

class IPNetAddr : public NetAddr {
public:

  IPNetAddr(const std::string &ip, uint16_t port);
  IPNetAddr(const std::string &addr);
  IPNetAddr(const sockaddr_in &addr);
  sockaddr *GetSockAddr();

  socklen_t GetSockLen();
  int GetFamily();

  std::string ToString();

	bool CheckValid() ;
	private:
	uint16_t port_;
	std::string ip_;
	sockaddr_in addr_;
};

} // namespace rocket