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


#ifndef __SST_MEMH_SIMPLEMEM_DATA_BACKENDCONVERTOR__
#define __SST_MEMH_SIMPLEMEM_DATA_BACKENDCONVERTOR__

#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemDataBackendConvertor : public MemBackendConvertor {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(SimpleMemDataBackendConvertor, "memHierarchy", "simpleMemDataBackendConvertor", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Converts a MemEventBase* for base MemBackend, including event payload", "SST::MemHierarchy::MemBackendConvertor")

    SST_ELI_DOCUMENT_PARAMS( MEMBACKENDCONVERTOR_ELI_PARAMS )

    SST_ELI_DOCUMENT_STATISTICS( MEMBACKENDCONVERTOR_ELI_STATS )

/* Begin class definition */
    SimpleMemDataBackendConvertor(Component *comp, Params &params);

    virtual bool issue( BaseReq* req );

    virtual void handleMemResponse( ReqId reqId ) {
        doResponse(reqId);
    }
};

}
}
#endif
