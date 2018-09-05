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
#include "sst/elements/memHierarchy/multiPortShim.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include "sst/elements/memHierarchy/memEvent.h"

using namespace SST;
using namespace SST::MemHierarchy;

MultiPortShim::MultiPortShim(Component* parent, Params &params) : CacheShim(parent, params) {

    std::string linkName = params.find<std::string>("cache_link", "");

    std::cout << "-------- MultiPortShim (" << getName() << ") --------\n" << std::endl;

    // Setup Links
    if (isPortConnected("cache_link")) {
        cacheLink_ = configureLink("cache_link", "50ps", new Event::Handler< MultiPortShim >(this, &MultiPortShim::handleRequest));
    } else {
        out_.fatal(CALL_INFO, -1, "%s, Error: no connected low_network_0 port. Please connect a cache to port 'low_network_0'\n", getName().c_str());
    }

    if (!isPortConnected("port_0")) out_.fatal(CALL_INFO, -1, "%s, Error: no connected CPU ports. Please connect a CPU to port 'thread0'.\n", getName().c_str());
    std::string linkname = "port_0";
    int linkid = 0;
    while (isPortConnected(linkname)) {
        SST::Link * link = configureLink(linkname, "50ps", new Event::Handler< MultiPortShim >(this, &MultiPortShim::handleResponse));
        highNetPorts_.push_back(link);
        linkid++;
        linkname = "port_" + std::to_string(linkid);
    }
}

void MultiPortShim::handleRequest(SST::Event * ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    Addr memAddr = event->getAddr();

    std::cout << "REQUEST " << std::endl;

//     cacheLink_->send(event);
    highNetPorts_[0]->send(event);
}

void MultiPortShim::handleResponse(SST::Event * ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    Addr memAddr = event->getAddr();

    std::cout << "RESPONSE " << std::endl;

    cacheLink_->send(event);
//     highNetPorts_[0]->send(event);

}
