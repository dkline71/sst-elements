// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ariel_H
#define _ariel_H

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/memEvent.h>
#include <sst/core/output.h>

#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>

using namespace std;
using namespace SST::Interfaces;

namespace SST {
class Event;

namespace ArielComponent {

class Ariel : public SST::Component {
public:

  Ariel(SST::ComponentId_t id, SST::Params& params);

  void setup()  { }
  void finish();

  void handleEvent(SST::Event* event);
private:
  Ariel();  // for serialization only
  Ariel(const Ariel&); // do not implement
  void operator=(const Ariel&); // do not implement

  virtual bool tick( SST::Cycle_t );
  int create_pinchild(char* prog_binary, char** arg_list);

  uint64_t max_inst;
  char* named_pipe;
  int pipe_id;
  std::string user_binary;
  Output* output;

  uint64_t memory_ops;
  uint64_t read_ops;
  uint64_t write_ops;
  uint64_t instructions;

  uint64_t split_read_ops;
  uint64_t split_write_ops;

  uint64_t cache_line_size;
  uint64_t page_size;
  uint64_t next_free_page_start;
  std::map<uint64_t, uint64_t>* page_table;
  std::map<MemEvent::id_type, MemEvent*> pending_requests;

  uint32_t core_count;
  uint32_t max_transactions;
  SST::Link** cache_link;
  MemEvent* pending_transaction;
  uint32_t pending_transaction_core;

  uint64_t translateAddress(uint64_t addr);
  void issue(uint64_t addr, uint32_t length, bool isRead, uint32_t thr);

  friend class boost::serialization::access;
  template<class Archive>
  void save(Archive & ar, const unsigned int version) const
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }

  template<class Archive>
  void load(Archive & ar, const unsigned int version) 
  {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
//    ar & BOOST_SERIALIZATION_NVP(max_trace_count);
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER()
 
};

}
}
#endif /* _ariel_H */
