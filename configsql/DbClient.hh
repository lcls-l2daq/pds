#ifndef Pds_DbClientSql_hh
#define Pds_DbClientSql_hh

#include "pds/config/DbClient.hh"

#include <mysql/mysql.h>

namespace Pds_ConfigDb {
  namespace Sql {
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

      /// Remove unreferenced data
      virtual void         purge();

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

      /// -- Browse Interface --
      /// Get a list of all XTC times matching src and type
      std::list<XtcEntryT>  getXTC(uint64_t source, 
				   unsigned type);

      /// Get the size of the XTC matching name, type, and time
      int                  getXTC(const XtcEntryT&);

      /// Get the payload of the XTC matching name, type, and time
      int                  getXTC(const XtcEntryT&,
				  void*    payload,
				  unsigned payload_size);
    private:
      void _updateKey(const std::string& alias,
                      std::list<ExptAlias>&  alist,
                      std::list<DeviceType>& dlist);
    private:
      enum Option { Lock, NoLock };
      Option          _lock;
      MYSQL*          _mysql;
    };
  };
};

#endif
