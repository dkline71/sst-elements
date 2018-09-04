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

#include "sst/elements/memHierarchy/cacheController.h"

namespace SST {
namespace MemHierarchy {

class CacheShim : public SST::SubComponent {

public:
/* Element Library Info */

/* Class Def */
    CacheShim(Component * comp, Params &params);
    ~CacheShim() { }


private:

};

class MultiPortShim : public CacheShim {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(MultiPortShim, "memHierarchy", "MultiPortShim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Used to provide a cache with multiple ports.", "SST::MemHierarchy::CacheShim")

    SST_ELI_DOCUMENT_PARAMS(
            {"num_ports",           "(uint) Number of ports.", "1"} )

    SST_ELI_DOCUMENT_PORTS(
          {"cache_link", "Link to cache", {"memHierarchy.MemEventBase"} },
          {"port_%(port)d", "Links to network", {"memHierarchy.MemEventBase"} } )

/* Class Def */
    MultiPortShim(Component * comp, Params &params);
    ~MultiPortShim() { }


private:

};

} //namespace memHierarchy
} //namespace SST

#endif
