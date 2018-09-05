// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright(c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "multiPortShim.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

using namespace SST;
using namespace SST::MemHierarchy;

MultiPortShim::MultiPortShim(Component* parent, Params &params) : CacheShim(parent, params) {

//     std::string linkName = params.find<std::string>("cache_link", "");
//     if (linkName == "")
//         dbg_.fatal(CALL_INFO, -1, "Param not specified(%s): cache_link - the name of the port that the shim is attached to. This should be set internally by components creating the shim.\n",
//                 getName().c_str());
//
//     // Setup Links
//     if (isPortConnected("low_network_0")) {
//         SST::Link* cacheLink_ = configureLink("low_network_0", "50ps", new Event::Handler< MultiPortShim >(this, &MultiPortShim::handleResponse));
//         highNetPorts_.push_back(cacheLink_);
//     } else {
//         out_.fatal(CALL_INFO, -1, "%s, Error: no connected low_network_0 port. Please connect a cache to port 'low_network_0'\n", getName().c_str());
//     }
//
//     if (!isPortConnected("thread0")) output.fatal(CALL_INFO, -1, "%s, Error: no connected CPU ports. Please connect a CPU to port 'thread0'.\n", getName().c_str());
//     std::string linkname = "thread0";
//     int linkid = 0;
//     while (isPortConnected(linkname)) {
//         SST::Link * link = configureLink(linkname, "50ps", new Event::Handler<MultiThreadL1, unsigned int>(this, &MultiThreadL1::handleRequest, linkid));
//         threadLinks.push_back(link);
//         linkid++;
//         linkname = "thread" + std::to_string(linkid);
//     }
//
//
//     /* Setup throughput limiting */
//     requestsPerCycle = params.find<uint64_t>("requests_per_cycle", 0);
//     responsesPerCycle = params.find<uint64_t>("responses_per_cycle", 0);

numLowNetPorts_ = 1;
numHighNetPorts_ = 1;
}

void MultiPortShim::init(unsigned int phase) {
    SST::Event *ev;

    for (int i = 0; i < numHighNetPorts_; i++) {
        while ((ev = highNetPorts_[i]->recvInitData())) {
            MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);

            if (memEvent) {
                dbg.debug(_L10_, "bus %s broadcasting upper event to lower ports (%d): %s\n", getName().c_str(), numLowNetPorts_, memEvent->getVerboseString().c_str());
                for (int k = 0; k < numLowNetPorts_; k++)
                    lowNetPorts_[k]->sendInitData(memEvent->clone());
            }
            delete memEvent;
        }
    }

//     for (int i = 0; i < numLowNetPorts_; i++) {
//         while ((ev = lowNetPorts_[i]->recvInitData())) {
//             MemEventInit* memEvent = dynamic_cast<MemEventInit*>(ev);
//             if (!memEvent) delete memEvent;
//             else if (memEvent->getCmd() == Command::NULLCMD) {
//                 dbg_.debug(_L10_, "bus %s broadcasting lower event to upper ports (%d): %s\n", getName().c_str(), numHighNetPorts_, memEvent->getVerboseString().c_str());
//                 mapNodeEntry(memEvent->getSrc(), lowNetPorts_[i]->getId());
//                 for (int i = 0; i < numHighNetPorts_; i++) {
//                     highNetPorts_[i]->sendInitData(memEvent->clone());
//                 }
//                 delete memEvent;
//             }
//             else{
//                 /*Ignore responses */
//                 delete memEvent;
//             }
//         }
//     }
}

void MultiPortShim::handleRequest(SST::Event * ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    Addr memAddr = event->getAddr();

}

void MultiPortShim::handleResponse(SST::Event * ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    Addr memAddr = event->getAddr();

}
