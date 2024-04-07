#pragma  once



#include <memory>
#include <string>
namespace rocket {

class AbstractProtocol {
public:
	using s_ptr = std::shared_ptr<AbstractProtocol>;
	virtual void test() = 0;

	~AbstractProtocol() = default;
	std::string GetReqId() {
		return req_id_;
	}
	void SetReqId(const std::string& req_id) {
		req_id_ = req_id;
	}

protected:
	std::string req_id_; // 请求号，唯一标识一个请求或者响应

private:
};
}