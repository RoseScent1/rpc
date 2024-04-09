#pragma once

#include "abstract_protocol.h"
#include <google/protobuf/service.h>
#include <memory>
#include <unordered_map>
namespace rocket {
class RpcDispatcher {
public:
	static RpcDispatcher* GetRpcDispatcher();
  void Dispatch(AbstractProtocol::s_ptr request,
                AbstractProtocol::s_ptr response);

	void RegisterService(std::shared_ptr<google::protobuf::Service> service);
private:
	bool ParseServiceFullName(const std::string &full_name, std::string &service_name,  std::string &method_name);
	std::unordered_map<std::string, std::shared_ptr<google::protobuf::Service>> service_table_;
};
} // namespace rocket