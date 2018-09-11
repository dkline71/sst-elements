// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MULTIPORTSHIM_H_
#define _MEMHIERARCHY_MULTIPORTSHIM_H_

#include <map>

#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>
#include <sst/core/warnmacros.h>
#include <sst/core/sst_types.h>
#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/memLinkBase.h"


namespace SST {
namespace MemHierarchy {

class MultiPortShim : public MemLinkBase {
public:
/* Element Library Info */
#define MULTIPORTSHIM_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
        {"num_ports",           "(uint) Number of ports.", "1"},\
        {"mem_link",            "(string) Set by parent component. Name of port connected to memory.", ""}, \
        {"line_size",           "(uint) Set by parent component. Size of cache line.", "64"}

    SST_ELI_REGISTER_SUBCOMPONENT(MultiPortShim, "memHierarchy", "MultiPortShim", SST_ELI_ELEMENT_VERSION(1,0,0),
                                  "Used to provide a cache with multiple ports.", "SST::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( MULTIPORTSHIM_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS(
        {"port_%(port)d", "Links to memory", {"memHierarchy.MemEventBase"} } )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"cpulink", "CPU-side link manager; for single-link caches use this one only", "SST::MemHierarchy::MemLinkBase"},
        {"memlink", "Memory-side link manager", "SST::MemHierarchy::MemLinkBase"})

    MultiPortShim(Component* comp, Params &params);
    ~MultiPortShim() { }

    /* Initialization functions for parent */
    void init(unsigned int phase);

    /* Send and receive functions for MemLink */
    void sendInitData(MemEventInit * ev);
    MemEventInit* recvInitData();
    void send(MemEventBase * ev);
    MemEventBase * recv();

private:
    Output out_;

    uint64_t lineSize_;
    uint64_t numPorts_;

    std::vector< SST::Link* > links_;

    std::map< std::string, LinkId_t >   nameMap_;
    std::map< LinkId_t, SST::Link* >    linkIdMap_;

    // Event handlers
    void handleResponse(SST::Event *event);
    void handleRequest(SST::Event *event);

    // Address & port handlers
    Addr toBaseAddr(Addr addr) { return (addr) & ~(lineSize_ - 1); }
    uint64_t getPortNum(Addr addr) { return (addr >> log2Of(lineSize_)) % numPorts_; }

};

} //namespace memHierarchy
} //namespace SST

#endif
