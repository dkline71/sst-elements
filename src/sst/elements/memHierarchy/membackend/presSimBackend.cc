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

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
PresSimBackend::PresSimBackend(Component *comp, Params &params) : SimpleMemBackend(comp, params){
    
    fixupParams( params, "clock", "backend.clock" );

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
bool PresSimBackend::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer received request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
    requestQueue.push_back(Req(id,addr,isWrite,numBytes));
    return true;
}

/* 
 * Issue as many requests as we can up to requestsPerCycle
 * by searching up to searchWindowSize requests
 */
bool PresSimBackend::clock(Cycle_t cycle) {
    
    if (!requestQueue.empty()) {
        
        int reqsIssuedThisCycle = 0;
        int reqsSearchedThisCycle = 0;
        
        std::list<Req>::iterator it = requestQueue.begin();
        
        while (it != requestQueue.end()) {
            
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

