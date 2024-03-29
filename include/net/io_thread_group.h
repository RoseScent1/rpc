#pragma once
#include <vector>
#include "io_thread.h"


namespace rocket {
class IOThreadGroup {
public:
 IOThreadGroup(int size);
 ~IOThreadGroup();
 void Start();
 void Join();
 IOThread* GetIOThread();
private:
	int size_;
	std::vector<IOThread *> iothread_groups_;
	int index_ ;
};
}