#pragma once

#include "log.h"
#include "net_addr.h"
#include "tcp_client.h"
#include <google/protobuf/arena.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <memory>
namespace rocket {

class RpcChannel : public google::protobuf::RpcChannel {
public:
  using s_ptr = std::shared_ptr<RpcChannel>;
  RpcChannel(NetAddr::s_ptr addr);
  ~RpcChannel();
  void CallMethod(const google::protobuf::MethodDescriptor *method,
                  google::protobuf::RpcController *controller,
                  const google::protobuf::Message *request,
                  google::protobuf::Message *response,
                  google::protobuf::Closure *done) override;
  void Init(std::shared_ptr<google::protobuf::RpcController> controller,
            std::shared_ptr<google::protobuf::Message> request,
            std::shared_ptr<google::protobuf::Message> response,
            std::shared_ptr<google::protobuf::Closure> closure);
  google::protobuf::RpcController *GetController() const;
  google::protobuf::Message *GetRequest() const;
  google::protobuf::Message *GetResponse() const;
  google::protobuf::Closure *GetClosure() const;
  TcpClient *GetClient() const;

private:
  TcpClient::s_ptr client_;
  std::shared_ptr<google::protobuf::RpcController> controller_;
  std::shared_ptr<google::protobuf::Message> request_;
  std::shared_ptr<google::protobuf::Message> response_;
  std::shared_ptr<google::protobuf::Closure> closure_;
  bool is_init_{false};
};
} // namespace rocket