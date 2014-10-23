#ifndef Pds_DbClient_hh
#define Pds_DbClient_hh

#include "pds/config/XtcClient.hh"
#include "pds/config/ClientData.hh"

namespace Pds_ConfigDb {
  class DbClient {
  public:
    virtual ~DbClient() {}
  public:
    static DbClient*             open  (const char* path);
  public:
    virtual int                  begin () = 0;
    virtual int                  commit() = 0;
    virtual int                  abort () = 0;
  public:
    virtual std::list<ExptAlias> getExptAliases() = 0;
    virtual int                  setExptAliases(const std::list<ExptAlias>&) = 0;
  public:
    virtual std::list<DeviceType> getDevices() = 0;
    virtual int                   setDevices(const std::list<DeviceType>&) = 0;
  public:
    /// Update the contents of all current keys
    virtual void                 updateKeys() = 0;

    /// Remove unreferenced data
    virtual void                 purge() = 0;

    /// Allocate an unused run key
    virtual unsigned             getNextKey() = 0;

    /// Get the list of known run keys
    virtual std::list<Key>       getKeys() = 0;
    
    /// Get the configurations used by a run key
    virtual std::list<KeyEntry>  getKey(unsigned) = 0;

    /// Get the configurations with timestamp used by a run key
    virtual std::list<KeyEntryT> getKeyT(unsigned) = 0;

    /// Set the configurations used by a run key
    virtual int                  setKey(const Key&,std::list<KeyEntry>) = 0;
  public:
    /// Get a list of all XTCs matching type
    virtual std::list<XtcEntry>  getXTC(unsigned type) = 0;

    /// Get the size of the XTC matching type and name
    virtual int                  getXTC(const    XtcEntry&) = 0;

    /// Get the payload of the XTC matching type and name
    virtual int                  getXTC(const    XtcEntry&,
                                        void*    payload,
                                        unsigned payload_size) = 0;
    /// Write the payload of the XTC matching type and name
    virtual int                  setXTC(const    XtcEntry&,
                                        const void*    payload,
                                        unsigned payload_size) = 0;

    /// -- Browse Interface --
    /// Get a list of all XTC times matching src and type
    virtual std::list<XtcEntryT>  getXTC(uint64_t source, 
					 unsigned type) = 0;

    /// Get the size of the XTC matching name, type, and time
    virtual int                  getXTC(const XtcEntryT&) = 0;

    /// Get the payload of the XTC matching name, type, and time
    virtual int                  getXTC(const XtcEntryT&,
                                        void*    payload,
                                        unsigned payload_size) = 0;

  };
};

#endif
