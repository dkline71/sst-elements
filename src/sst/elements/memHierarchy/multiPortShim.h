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

#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/cacheController.h"

namespace SST {
namespace MemHierarchy {

class CacheShim : public MemLinkBase {

public:
/* Element Library Info */
#define CACHESHIM_ELI_PARAMS MEMLINKBASE_ELI_PARAMS, \
    { "debug",              "(int) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},\
    { "debug_level",        "(int) Debug verbosity level. Between 0 and 10", "0"}

    CacheShim(Component* comp, Params &params);
    ~CacheShim();

    void sendInitData(MemEventInit * ev);
    MemEventInit* recvInitData();
    void send(MemEventBase * ev);

protected:


};

class MultiPortShim : public CacheShim {
public:
/* Element Library Info */
#define MULTIPORTSHIM_ELI_PARAMS CACHESHIM_ELI_PARAMS, \
            {"num_ports",           "(uint) Number of ports.", "1"},\
            {"cache_link",          "(string) Set by parent component. Name of port connected to cache.", ""}

    SST_ELI_REGISTER_SUBCOMPONENT(MultiPortShim, "memHierarchy", "MultiPortShim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Used to provide a cache with multiple ports.", "SST::MemLinkBase")

    SST_ELI_DOCUMENT_PARAMS( MULTIPORTSHIM_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS(
          {"cache_link", "Link to cache", {"memHierarchy.MemEventBase"} },
          {"port_%(port)d", "Links to network", {"memHierarchy.MemEventBase"} } )

    MultiPortShim(Component* comp, Params &params);
    ~MultiPortShim() { }

    // Event handlers
    void handleResponse(SST::Event *event);
    void handleRequest(SST::Event *event);

    // Init
    void init(unsigned int phase);

    uint32_t numLowNetPorts_;
    uint32_t numHighNetPorts_;

private:
    std::vector<SST::Link*>         highNetPorts_;
    std::vector<SST::Link*>         lowNetPorts_;

};

} //namespace memHierarchy
} //namespace SST

#endif
