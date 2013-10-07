#ifndef PDS_SEGWIRESETTINGS_HH
#define PDS_SEGWIRESETTINGS_HH

#include "StreamParams.hh"
#include "pdsdata/psddl/alias.ddl.h"
#include <list>

using Pds::Alias::SrcAlias;

namespace Pds {

class InletWire;

class SegWireSettings {
public:
  virtual ~SegWireSettings() {}

  virtual void connect (InletWire& inlet,
			StreamParams::StreamType s,
			int interface) = 0;

  virtual const std::list<Src>& sources() const = 0;

  virtual const std::list<SrcAlias>* pAliases() const {
    return ((std::list<SrcAlias>*)NULL);
  }

  virtual unsigned max_event_size () const { return 16*1024*1024; }
  virtual unsigned max_event_depth() const { return 32; }
};
}
#endif
