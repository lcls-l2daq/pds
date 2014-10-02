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

    bool operator==(const XtcEntry& o) const { 
      return type_id.value()==o.type_id.value() &&
	name == o.name; 
    }
  };

  class XtcEntryT {
  public:
    time_t      time;
    std::string stime;
    XtcEntry    xtc;
  };

  class KeyEntry {
  public:
    uint64_t    source;
    XtcEntry    xtc;

    bool operator==(const KeyEntry& o) const {
      return source==o.source && xtc==o.xtc; 
    }
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
