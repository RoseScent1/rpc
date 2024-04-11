#include "net_addr.h"
#include "log.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
namespace rocket {
IPNetAddr::IPNetAddr(const std::string &ip, uint16_t port)
    : port_(port), ip_(ip) {
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = inet_addr(ip.c_str());
  addr_.sin_port = htons(port);
}
IPNetAddr::IPNetAddr(const std::string &addr) {
  int pos = addr.find_first_of(":");
  if (pos == addr.npos) {
    RPC_ERROR_LOG("addr error addr = %s", addr.c_str());
    return;
  }
  ip_ = addr.substr(0, pos);
  port_ = std::atoi(addr.substr(pos + 1, addr.size() - pos - 1).c_str());

  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = inet_addr(ip_.c_str());
  addr_.sin_port = htons(port_);
}

IPNetAddr::IPNetAddr(const sockaddr_in &addr) : addr_(addr) {
  ip_ = inet_ntoa(addr.sin_addr);
  port_ = ntohs(addr.sin_port);
}


IPNetAddr::~IPNetAddr(){
	// RPC_INFO_LOG("~IPNetAddr");
}

sockaddr *IPNetAddr::GetSockAddr() {
  return reinterpret_cast<sockaddr *>(&addr_);
}

socklen_t IPNetAddr::GetSockLen() { return sizeof(addr_); }

int IPNetAddr::GetFamily() { return addr_.sin_family; }

std::string IPNetAddr::ToString() { return ip_ + ":" + std::to_string(port_); }

bool IPNetAddr::CheckValid() {
  return !ip_.empty() && inet_addr(ip_.c_str()) != INADDR_NONE;
}

} // namespace rocket