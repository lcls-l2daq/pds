#ifndef Pds_ClientData_hh
#define Pds_ClientData_hh

#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <list>
#include <string>

namespace Pds_ConfigDb {

  /**
   **/
  class XtcEntry {
  public:
    Pds::TypeId type_id;
    std::string name;
  };

  class KeyEntry {
  public:
    uint64_t    source;
    XtcEntry    xtc;
  };

  class Key {
  public:
    unsigned    key;
    time_t      time;
    std::string name;
  };

  class DeviceEntryRef {
  public:
    std::string device;
    std::string name;
  };

  class DeviceEntries {
  public:
    std::string name;
    std::list<XtcEntry> entries;
  };

  class DeviceType {
  public:
    std::string name;
    std::list<DeviceEntries> entries;
    std::list<uint64_t>      sources;
  };

  class ExptAlias {
  public:
    std::string name;
    unsigned    key;
    std::list<DeviceEntryRef> devices;
  };

};

#endif
