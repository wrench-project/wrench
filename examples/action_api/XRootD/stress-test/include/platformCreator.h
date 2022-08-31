#ifndef platformCreator_H
#define platformCreator_H

#include <wrench/services/storage/xrootd/XRootD.h>
#include <wrench-dev.h>
struct Return{
	std::shared_ptr<wrench::XRootD::Node> root;
	vector<std::shared_ptr<wrench::XRootD::Node>> fileServers;

};
class PlatformCreator {

public:
    PlatformCreator(wrench::XRootD::XRootD &metavisor,
		double density,
		int leafs) : metavisor(metavisor),density(density),leafs(leafs) {}

    void operator()() ;

    wrench::XRootD::XRootD &metavisor;
	double density;
	int leafs;
	//created in init
	std::shared_ptr<Return> ret;
};
#endif
