#pragma  once



#include <memory>
namespace rocket {

class AbstractProtocol {
public:
	using s_ptr = std::shared_ptr<AbstractProtocol>;
	
private:
};
}