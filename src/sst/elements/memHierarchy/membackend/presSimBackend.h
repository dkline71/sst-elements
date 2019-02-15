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


#ifndef _H_SST_MEMH_PRES_SIM_BACKEND
#define _H_SST_MEMH_PRES_SIM_BACKEND

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/elements/memHierarchy/membackend/memBackend.h>
#include <list>

#include <PresSim.h>

namespace SST {
namespace MemHierarchy {

class PresSimBackend : public SimpleMemDataBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(PresSimBackend,
        "memHierarchy", 
        "presSim", 
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Reliability Module, currently just a pass-through", 
        "SST::MemHierarchy::MemBackend")
    
    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"max_issue_per_cycle", "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
            {"search_window_size",  "Maximum number of requests to search each cycle. 0 or negative is unlimited.", "-1"},
            {"backend",             "Backend memory system", "memHierarchy.simpleDRAM"},
			{"config",              "Configuration parameters for PresSim"} )

/* Begin class definition */
    ~PresSimBackend();
    PresSimBackend(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes, vector<uint8_t> data );
    void setup();
    void finish();
    bool clock(Cycle_t cycle);
    virtual const std::string& getClockFreq() { return backend->getClockFreq(); }

private:
    void handleMemReponse( ReqId id ) {
        SimpleMemDataBackend::handleMemResponse( id );
    }
    struct Req {
        Req( ReqId id, Addr addr, bool isWrite, unsigned numBytes, uint64_t delay ) :
            id(id), addr(addr), isWrite(isWrite), numBytes(numBytes), delay(delay)
        { }
        ReqId id;
        Addr addr;
        bool isWrite;
        unsigned numBytes;
		uint64_t delay;
    };
	Component* owner;
    PresSim::ReliabilitySimulator *relSystem;
    SimpleMemBackend* backend;
	uint64_t ps_time;
    int reqsPerCycle;       // Number of requests to issue per cycle (max) -> memCtrl limits how many we accept
    int searchWindowSize;   // Number of requests to search when looking for requests to issue
    std::list<Req> requestQueue;   // List of requests waiting to be issued
};

}
}

#endif
