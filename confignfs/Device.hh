#ifndef Pds_ConfigDb_DevicesNfs_hh
#define Pds_ConfigDb_DevicesNfs_hh

#include "pds/confignfs/Table.hh"
#include "pds/config/DeviceEntry.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <time.h>
#include <list>
#include <string>

namespace Pds_ConfigDb {

  class UTypeName;

  namespace Nfs {
    class XtcTable;
    class Path;

    class Device {
    public:
      Device();
      Device( const std::string& path, 
              const std::string& name,
              const std::list<DeviceEntry>& src_list );
    public:
      void load(const char*&);
      void save(char*&) const;
    public:
      bool operator==(const Device&) const;
    public:
      const std::string& name() const { return _name; }

      //  Table of {device alias, key, {config_type,filename}}
      const Table&  table() const { return _table; }
      Table& table() { return _table; }

      //  List of Pds::Src entries
      const std::list<DeviceEntry>& src_list() const { return _src_list; }
      std::list<DeviceEntry>& src_list() { return _src_list; }
    public:
      std::string keypath (const std::string& path, const std::string& key) const;
      std::string xtcpath (const std::string& path, const UTypeName& uname, const std::string& entry) const;
      std::string typepath(const std::string& path, const std::string& key, const UTypeName& entry) const;
      std::string typelink(const UTypeName& name, const std::string& entry) const;
    public:
      void   update_keys (const Path&, XtcTable&, time_t);
    public:    // Deprecated
      bool   validate_key_file(const std::string& config, const std::string& path);
      bool   update_key_file  (const std::string& config, const std::string& path);
    private:
      bool _check_config_file(const TableEntry* entry, const std::string& path, const std::string& key);
      void _make_config_file (const TableEntry* entry, const std::string& path, const std::string& key);
    private:
      std::string    _name;
      Table          _table;
      std::list<DeviceEntry> _src_list;
    };
  };
};

#endif
