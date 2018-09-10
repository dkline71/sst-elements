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

#ifndef _MEMHIERARCHY_CACHESHIM_H_
#define _MEMHIERARCHY_CACHESHIM_H_

#include <sst/core/event.h>
#include <sst/core/subcomponent.h>
#include <sst/core/warnmacros.h>
#include <sst/core/sst_types.h>
#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memLinkBase.h"


namespace SST {
namespace MemHierarchy {

class CacheShim : public MemLinkBase {
public:
/* Element Library Info */
#define CACHESHIM_ELI_PARAMS MEMLINKBASE_ELI_PARAMS

    CacheShim(Component * comp, Params &params);
    ~CacheShim() { }

    /* Initialization functions for parent */
    virtual void init(unsigned int phase);

    /* Send and receive functions for MemLink */
    virtual void sendInitData(MemEventInit * ev);
    virtual MemEventInit* recvInitData();
    virtual void send(MemEventBase * ev);
    virtual MemEventBase * recv();

    /* Debug */
    virtual void printStatus(Output &out) {
        out.output("  MemHierarchy::MemLink: No status given\n");
    }

};

} //namespace memHierarchy
} //namespace SST

#endif
