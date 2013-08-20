#ifndef PDS_SEGWIRESETTINGS_HH
#define PDS_SEGWIRESETTINGS_HH

#include "StreamParams.hh"
#include "pdsdata/xtc/SrcAlias.hh"
#include <list>

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
};
}
#endif
