// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst/core/sst_config.h>
#include <sst/elements/memHierarchy/util.h>
#include "membackend/presSimBackend.h"
#include "sst/elements/memHierarchy/memoryController.h"
#include <math.h> 
#include <map>

using namespace SST;
using namespace SST::MemHierarchy;

//Want to be copy by value, not reference
static uint64_t clock_ps_convertor(string clk){
	
	std::map<string,uint64_t> f = {{"THz",1}, {"Thz", 1}, {"GHz", 1000}, {"Ghz",1000}, {"MHz",1000000}, {"Mhz", 1000000},
								{"KHz", 1000000000}, {"Khz", 1000000000}};
	//, {"Hz", 1000000000000}};
 
	std::map<string, uint64_t> s = {{"ps", 1}, {"ns", 1000}, {"us", 1000000}, {"ms", 1000000000}}; 
	
	uint64_t factor = 0;
	bool div = false;
	for (auto fe = f.begin(); fe!=f.end(); ++fe)
	{
		if (clk.find(fe->first)!=std::string::npos) {
			factor = fe->second;
			div = true;
			clk = clk.substr(0, clk.find(fe->first));
			break;
		}
	}	
	if (factor==0)
	{
		if (clk.find("Hz")!=std::string::npos || clk.find("hz")!=std::string::npos)
		{
			div = true;
			factor = 1000000000000;
			clk = clk.substr(0, clk.find("Hz"));
		}
	}
	if (factor == 0)
	{
    	for (auto se = s.begin(); se!=s.end(); ++se)
    	{
        	if (clk.find(se->first)!=std::string::npos) {
            	factor = se->second;
				clk = clk.substr(0, clk.find(se->first));
        		break;
			}
    	} 
	}
	if (factor == 0)
	{
		if (clk.find("s")!=std::string::npos)
		{
			factor = 1000000000000;
			clk = clk.substr(0, clk.find("s"));
		}
	}
	if (factor == 0)
		return 0;

	std::string::size_type sz;
	double val = std::stod(clk, &sz);
	if (div)
		val = 1/val;
	val = (uint64_t) ceil(val*factor);
	
	return val;


	
}
/*------------------------------- Simple MemData Backend ------------------------------- */
PresSimBackend::PresSimBackend(Component *comp, Params &params) : SimpleMemDataBackend(comp, params), owner(comp){
    
    fixupParams( params, "clock", "backend.clock" );

	string clock_str = params.find<string>("clock", NO_STRING_DEFINED);
	ps_time = clock_ps_convertor(clock_str);
	if (ps_time == 0)
		output->fatal(CALL_INFO, -1, "Given clock invalid\n");
	reqsPerCycle = params.find<int>("max_issue_per_cycle", -1);
    searchWindowSize = params.find<int>("search_window_size", -1);
	std::string config_file = params.find<std::string>("config", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == config_file)
        output->fatal(CALL_INFO, -1, "Model must define a 'config' file parameter\n");	
    //Initialize Reliability System
    relSystem = new PresSim::ReliabilitySimulator(config_file);

    // Create our backend & copy 'mem_size' through for now
    std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
    Params backendParams = params.find_prefix_params("backend.");
    backendParams.insert("mem_size", params.find<std::string>("mem_size"));
    backend = dynamic_cast<SimpleMemBackend*>(loadSubComponent(backendName, backendParams));
    using std::placeholders::_1;
    backend->setResponseHandler( std::bind( &PresSimBackend::handleMemResponse, this, _1 )  );

	//If don't want the default clock, overwrite it here
}

PresSimBackend::~PresSimBackend(){
	delete relSystem;
}
bool PresSimBackend::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes, std::vector<uint8_t> data_in ) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer received request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
	//Goal- get delay from PresSim, don't start issuing until delay is finished
	std::vector<uint8_t> data;
	static_cast<MemController*>(owner)->readData(addr, numBytes, data);
	unsigned long long delay = 0;
	if (isWrite)
		
		delay = relSystem->perform_write(addr, data, data_in);
	else
		delay = relSystem->perform_read(addr, data);
    requestQueue.push_back(Req(id,addr,isWrite,numBytes,delay));
	return true;
}


/* 
 * Issue as many requests as we can up to requestsPerCycle
 * by searching up to searchWindowSize requests
 */
bool PresSimBackend::clock(Cycle_t cycle) {

	relSystem->tick();    
    if (!requestQueue.empty()) {
        
        int reqsIssuedThisCycle = 0;
        int reqsSearchedThisCycle = 0;
        
        std::list<Req>::iterator it = requestQueue.begin();

        while (it != requestQueue.end()) {

			if (it->delay>ps_time)
			{
				(*it).delay -= ps_time;
				it++;
				continue;
			}
			else
				(*it).delay = 0;
			//assumes backend is SimpleMemBackend
            bool issued = backend->issueRequest( (*it).id, (*it).addr, (*it).isWrite, (*it).numBytes );
            
            if (issued) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer issued request for 0x%" PRIx64 "\n", (Addr)(*it).addr);
#endif
                reqsIssuedThisCycle++;
                it = requestQueue.erase(it);
                if (reqsIssuedThisCycle == reqsPerCycle) break;
            } else {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer could not issue 0x%" PRIx64 "\n", (Addr)(*it).addr);
#endif
                it++;
            }
        
            reqsSearchedThisCycle++;
            if (reqsSearchedThisCycle == searchWindowSize) break;
        }
    }
	
    bool unclock = backend->clock(cycle);
    return false;
}


/*
 * Call throughs to our backend
 */

void PresSimBackend::setup() {
    backend->setup();
}

void PresSimBackend::finish() {
    backend->finish();
}

