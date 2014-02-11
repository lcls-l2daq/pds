#ifndef Pds_DbClientNfs_hh
#define Pds_DbClientNfs_hh

#include "pds/config/DbClient.hh"
#include "pds/confignfs/Path.hh"
#include "pds/confignfs/Table.hh"
#include "pds/confignfs/Device.hh"

namespace Pds_ConfigDb {
  namespace Nfs {
    class DbClient : public Pds_ConfigDb::DbClient {
    private:
      DbClient(const char*);
    public:
      ~DbClient();
    public:
      static DbClient*  open  (const char* path);
    public:
      void                 lock  ();
      void                 unlock();
    public:
      int                  begin ();
      int                  commit();
      int                  abort ();
    public:
      std::list<ExptAlias> getExptAliases();
      int                  setExptAliases(const std::list<ExptAlias>&);
    public:
      std::list<DeviceType> getDevices();
      int                   setDevices(const std::list<DeviceType>&);
    public:
      /// Update the contents of all current keys
      void                 updateKeys();

      /// Allocate an unused run key
      unsigned             getNextKey();

      /// Get the list of known run keys
      std::list<Key>       getKeys();
    
      /// Get the configurations used by a run key
      std::list<KeyEntry>  getKey(unsigned);

      /// Set the configurations used by a run key
      int                  setKey(const Key&,std::list<KeyEntry>);
    public:
      /// Get a list of all XTCs matching type
      std::list<XtcEntry>  getXTC(unsigned type);

      /// Get the size of the XTC matching type and name
      int                  getXTC(const    XtcEntry&);

      /// Get the payload of the XTC matching type and name
      int                  getXTC(const    XtcEntry&,
                                  void*    payload,
                                  unsigned payload_size);
      /// Write the payload of the XTC matching type and name
      int                  setXTC(const    XtcEntry&,
                                  const void*    payload,
                                  unsigned payload_size);
    private:
      void _close();
      void _open ();
      const Device* _device(const std::string&) const;
    private:
      Path            _path;
      enum Option { Lock, NoLock };
      Option          _lock;
      FILE*           _f;
    };
  };
};

#endif
