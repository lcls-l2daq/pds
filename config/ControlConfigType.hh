#ifndef Pds_ControlConfigType_hh
#define Pds_ControlConfigType_hh

#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/psddl/control.ddl.h"

#include <list>

typedef Pds::ControlData::ConfigV3 ControlConfigType;

static Pds::TypeId _controlConfigType(Pds::TypeId::Id_ControlConfig,
				      ControlConfigType::Version);

namespace Pds {
  namespace ControlConfig {

    class L3TEvents {
    public:
      L3TEvents(unsigned e) : _events(e) {}
      operator unsigned() const { return _events; }
    private:
      unsigned _events;
    };

    ControlConfigType* _new(void*);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            const Pds::ClockTime&);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            unsigned events);
    ControlConfigType* _new(void*, 
                            const std::list<Pds::ControlData::PVControl>&,
                            const std::list<Pds::ControlData::PVMonitor>&,
                            const std::list<Pds::ControlData::PVLabel>&,
                            L3TEvents events);
  }
}

#endif
