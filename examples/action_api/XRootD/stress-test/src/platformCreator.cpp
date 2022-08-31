#include "../include/platformCreator.h"
#include <math.h>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <wrench/services/storage/xrootd/XRootD.h>

#include <wrench/services/storage/xrootd/Node.h>

using namespace std;
const string NETWORK_SPEED="1GBps";
const string CPU_SPEED="1Gf";
const string NETWORK_LATENCY="10ms";

namespace sg4 = simgrid::s4u;
struct Tree{
	vector<std::shared_ptr<Tree>> children;
	Tree* parrent;
	sg4::Host* host;
};
void PlatformCreator::operator()() {

        // Create the top-level zone
        auto zone = sg4::create_full_zone("AS0");
        auto loopback = zone->create_link("loopback", "1000EBps")->set_latency("0us")-> set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)->seal();
        auto fatpipe = zone->create_link("fatpipe", "300MBps")-> set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)->set_latency("100ms")->seal();
                // Create a ComputeHost
        auto user = zone->create_host("user", "1Mf");
        user->set_core_count(10);
        user->set_property("ram", "16GB");
		std::srand(std::time(nullptr));
		//create graph
		Tree tRoot;
		vector<std::shared_ptr<Tree>> prevLevel=vector<std::shared_ptr<Tree>>(leafs);
		for(int i=0;i<leafs;i++){
			prevLevel[i]=(std::make_shared<Tree>());
		}
		while(true){
			unsigned int level=round(prevLevel.size()/(64*density));
			if(level*64<prevLevel.size()){
				level=(prevLevel.size()/64)+1;//I dont think this will ever happen, but it might due to rounding weirdness, this will just insure that there are always enough nodes
			}
			if(level<=1){
				for(unsigned int i=0;i<prevLevel.size();i++){
					prevLevel[i]->parrent=&tRoot;
					tRoot.children.push_back(prevLevel[i]);
				}
				break;
			}
			vector<std::shared_ptr<Tree>> parrents=vector<std::shared_ptr<Tree>>(level);
			for(unsigned int i=0;i<level;i++){
				parrents[i]=(std::make_shared<Tree>());
			}
			int unallocated=0;
			for(unsigned int i=0;i<prevLevel.size();i++){
				int n=std::rand()/((RAND_MAX + 1u)/level);
				if(parrents[n]->children.size()<64){
					prevLevel[i]->parrent=parrents[n].get();
					parrents[n]->children.push_back(prevLevel[i]);
				}else{
					unallocated++;
				}
				
			}
			int j=0;
			for(unsigned int i=0;i<prevLevel.size()&&unallocated>0;i++){
				if(!prevLevel[i]->parrent){
					while(parrents[j]->children.size()>=64){
						j=(j+1)%parrents.size();
					prevLevel[i]->parrent=parrents[j].get();
					parrents[j]->children.push_back(prevLevel[i]);
					}
				}
			}

			prevLevel=parrents;
			
		}
		
		
		auto current=&tRoot;
		
		 tRoot.host = zone->create_host("root", "1Mf");
					tRoot.host->set_core_count(10);
					tRoot.host->set_property("ram", "16GB");

					//add loopback
					{
					sg4::LinkInRoute network_link_in_route{loopback};
            zone->add_route(tRoot.host->get_netpoint(),
                            tRoot.host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
                    
                    //add userpipe
					{
					sg4::LinkInRoute network_link_in_route{fatpipe};
            zone->add_route(tRoot.host->get_netpoint(),
                            user->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
		ret->root=metavisor.createSupervisor("root");			
		auto currentNode=ret->root.get();
		int superCount=0;
		int leafCount=0;
		
		int linkCount=0;
		std::stack<int> backtrack;
		backtrack.push(0);
		std::vector<std::vector<sg4::LinkInRoute>> routes;
		std::vector<sg4::Host*> parrents;
		parrents.push_back(tRoot.host);

		while(!backtrack.empty()){
			unsigned int index=backtrack.top();
			if(current->children.size()>index){//go down
				if(current->children[index]->children.size()>0){//next target found
					
					
					++superCount;
					// Create a ComputeHost
					auto compute_host = zone->create_host("super"+to_string(superCount), CPU_SPEED);
					compute_host->set_core_count(10);
					compute_host->set_property("ram", "16GB");

					//add loopback
					{
					sg4::LinkInRoute network_link_in_route{loopback};
            zone->add_route(compute_host->get_netpoint(),
                            compute_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
                    
                    //add userpipe
					{
					sg4::LinkInRoute network_link_in_route{fatpipe};
            zone->add_route(compute_host->get_netpoint(),
                            user->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
					
                  current->children[index]->host=compute_host;
					{
						//add parrent routes
						 auto treenet = zone->create_link("link"+to_string(linkCount++), NETWORK_SPEED)->set_latency(NETWORK_LATENCY)->seal();
						sg4::LinkInRoute newLink{treenet};
						if(routes.size()>0){
							std::vector<sg4::LinkInRoute> tmp=routes[routes.size()-1];
							tmp.push_back(newLink);
							routes.push_back(tmp);
						}else{
							routes.push_back(std::vector<sg4::LinkInRoute>());
							routes[routes.size()-1].push_back(newLink);
						}
						for(unsigned int i=0;i<routes.size();i++){
							zone->add_route( 
								current->children[index]->host->get_netpoint(),
								parrents[i]->get_netpoint(),
								nullptr,
								nullptr,
								routes[i]);
						}
						
					}
				  parrents.push_back(current->children[index]->host);
				  auto next=metavisor.createSupervisor("super"+to_string(superCount));
					currentNode->addChild(next);
					backtrack.push(0);
				  currentNode=next.get();
				  current=current->children[index].get();
				}else{//create leaf and move sideways 
					
					++leafCount;
					// Create the WMSHost host with its disk
					auto wms_host = zone->create_host("leaf"+to_string(leafCount), CPU_SPEED);
					wms_host->set_core_count(1);
					auto wms_host_disk = wms_host->create_disk("hard_drive",
						                                       "100MBps",
						                                       "100MBps");
					wms_host_disk->set_property("size", "5000GiB");
					wms_host_disk->set_property("mount", "/");
					
					//add loopback
					{
					sg4::LinkInRoute network_link_in_route{loopback};
            zone->add_route(wms_host->get_netpoint(),
                            wms_host->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
                    
                    //add userpipe
					{
					sg4::LinkInRoute network_link_in_route{fatpipe};
            zone->add_route(wms_host->get_netpoint(),
                            user->get_netpoint(),
                            nullptr,
                            nullptr,
                            {network_link_in_route});
                           
                    }
					current->children[index]->host=wms_host;
					{
						//add parrent routes
						 auto treenet = zone->create_link("link"+to_string(linkCount++), NETWORK_SPEED)->set_latency(NETWORK_LATENCY)->seal();
						sg4::LinkInRoute newLink{treenet};
						if(routes.size()>0){
							std::vector<sg4::LinkInRoute> tmp=routes[routes.size()-1];
							tmp.push_back(newLink);
							routes.push_back(tmp);
						}else{
							routes.push_back(std::vector<sg4::LinkInRoute>());
							routes[routes.size()-1].push_back(newLink);
						}
						for(unsigned int i=0;i<routes.size();i++){
							zone->add_route( 
								current->children[index]->host->get_netpoint(),
								parrents[i]->get_netpoint(),
								nullptr,
								nullptr,
								routes[i]);
						}
						
						routes.pop_back();
						auto next=metavisor.createStorageServer("leaf"+to_string(leafCount),"/",{},{});
					currentNode->addChild(next);
					ret->fileServers.push_back(next);
					
					index++;
					backtrack.pop();
					backtrack.push(index);
					}
				}
			
			}else{//go up and sideways
				backtrack.pop();
				if(routes.size()>0){
					routes.pop_back();
						
					parrents.pop_back();
				}
				if(!backtrack.empty()){
					int index=backtrack.top();
					index++;
					backtrack.pop();
					backtrack.push(index);
					current=current->parrent;
					currentNode=currentNode->getParent();
				}
			}
		}

        zone->seal();
    }
