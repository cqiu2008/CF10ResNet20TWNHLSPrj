#ifndef __CRESNET20__HPP__
#define __CRESNET20__HPP__

#include "network.hpp"
#include "cnv_layer.hpp"
#include "../src/common.hpp"
#include "../src/logger.hpp"
class cresnet20:public network{
public:
	cresnet20();
	virtual ~cresnet20();
	virtual void CreateNetwork();
	virtual void StatMemoryUsage();
//	virtual void OptimizeNetwork();
private:
	cresnet20(const cresnet20& cres20);
	cresnet20& operator=(const cresnet20& cres20);

};





#endif
