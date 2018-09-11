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

MultiPortShim::MultiPortShim(Component* parent, Params &params) : MemLinkBase(parent, params) {
    out_.init("", 1, 0, Output::STDOUT);

    lineSize_ = params.find< uint64_t >("line_size", 64);
//     numPorts_ = params.find< uint64_t >("cacheShim.num_ports", 1);
    numPorts_ = 0;

    std::cout << "-------- MultiPortShim (" << getName() << "), " << lineSize_ << " " << numPorts_ << " --------\n" << std::endl;

    // Configure link
    std::string latency = params.find<std::string>("latency", "50ps");

    std::string linkname = "port_0";
    while( isPortConnected(linkname) ) {
        SST::Link* link = configureLink(linkname, latency, new Event::Handler< MultiPortShim >(this, &MultiPortShim::recvNotify));
        if (!link) {
            dbg.fatal(CALL_INFO, -1, "%s MemLink: Unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
        }

        dbg.debug(_L10_, "%s memLink info is: Name: %s, addr: %" PRIu64 ", id: %" PRIu32 "\n",
                getName().c_str(), info.name.c_str(), info.addr, info.id);

        links_.push_back(link);

        linkIdMap_[links_[numPorts_]->getId()] = links_[numPorts_];
        numPorts_++;
        linkname = "port_" + std::to_string(numPorts_);
    }

//     for( auto i = 0; i < numPorts_; i++ ) {
//         linkname = "port_" + std::to_string(i);
//         SST::Link* link = configureLink(linkname, latency, new Event::Handler< MultiPortShim >(this, &MultiPortShim::recvNotify));
//
//         if (!link) {
//             dbg.fatal(CALL_INFO, -1, "%s MemLink: Unable to configure link on port '%s'\n", getName().c_str(), linkname.c_str());
//         }
//
//         dbg.debug(_L10_, "%s memLink info is: Name: %s, addr: %" PRIu64 ", id: %" PRIu32 "\n",
//                 getName().c_str(), info.name.c_str(), info.addr, info.id);
//
//         links_.push_back(link);
//     }
}

void MultiPortShim::init(unsigned int phase) {
    SST::Event * ev;

    for( auto i = 0; i < numPorts_; i++ ) {
        while ((ev = links_[i]->recvInitData())) {
            MemEventInit * mEv = dynamic_cast<MemEventInit*>(ev);
            if (mEv) {
                if (mEv->getInitCmd() == MemEventInit::InitCommand::Region) {
                    MemEventInitRegion * mEvRegion = static_cast<MemEventInitRegion*>(mEv);
                    dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvRegion->getVerboseString().c_str());

                    EndpointInfo epInfo;
                    epInfo.name = mEvRegion->getSrc();
                    epInfo.addr = 0;
                    epInfo.id = 0;
                    epInfo.region = mEvRegion->getRegion();
                    addSource(epInfo);
                    addDest(epInfo);

                    if (mEvRegion->getSetRegion() && acceptRegion) {
                        dbg.debug(_L10_, "\tUpdating local region\n");
                        info.region = mEvRegion->getRegion();
                    }
                    delete ev;
                } else { /* No need to filter by source since this is a direct link */
                    initReceiveQ.push(mEv);
                }
            } else
                delete ev;
        }
    }
}


void MultiPortShim::sendInitData(MemEventInit * event) {
    for( auto i = 0; i < numPorts_; i++ ) {
        dbg.debug(_L10_, "%s sending init message: %s\n", getName().c_str(), event->getVerboseString().c_str());
        links_[i]->sendInitData(event);
    }
}


MemEventInit* MultiPortShim::recvInitData() {
    MemEventInit * me = nullptr;
    if (!initReceiveQ.empty()) {
        me = initReceiveQ.front();
        initReceiveQ.pop();
    }

    return me;
}


void MultiPortShim::send(MemEventBase *ev) {
    for( auto i = 0; i < numPorts_; i++ ) {
        dbg.debug(_L4_,"SEND (%s). event: (%s)\n", getName().c_str(), ev->getBriefString().c_str());
        links_[i]->send(ev);
    }
}


MemEventBase* MultiPortShim::recv() {
    SST::Event * ev = links_[0]->recv();

    dbg.debug(_L4_,"RECV (%s). event: (%s)\n", getName().c_str(), dynamic_cast<MemEventBase*>(ev)->getBriefString().c_str());


    MemEventBase * mEv = dynamic_cast<MemEventBase*>(ev);
    if (mEv) return mEv;

    if (ev) delete ev;

    return nullptr;
}









