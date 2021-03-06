#ifndef PDS_IPIMBSERVER
#define PDS_IPIMBSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/IpimbConfigType.hh"

#include "pds/ipimb/IpimBoard.hh"

#include <string>

#include "pdsdata/psddl/ipimb.ddl.h"
typedef Pds::Ipimb::DataV1 OldIpimbDataType;
static Pds::TypeId  oldIpimbDataType(Pds::TypeId::Id_IpimbData,
                                  OldIpimbDataType::Version);

namespace Pds {

  class IpimbServer : public EbServer, public EbCountSrv {
  public:
    IpimbServer(const Src& client, const bool c01);
    virtual ~IpimbServer() {}
    
  public:
    //  Eb interface
    void        dump    (int detail)   const;
    bool        isValued()             const;
    const Src&  client  ()             const;
    //  EbSegment interface
    const Xtc&  xtc   () const;
    unsigned    offset() const;
    unsigned    length() const;
  public:
    //  Eb-key interface
    EbServerDeclare;
  public:
    //  Server interface
    int      pend  (int flag = 0);
    int      fetch (char* payload, int flags);
  public:
    unsigned count() const;
    void setIpimb(IpimBoard* ipimb, char* serialDevice, int baselineMode, int polarity);

  public:
    unsigned configure(IpimbConfigType& config, std::string&);
    unsigned unconfigure();

  private:
    Xtc _xtc;
    unsigned _count;
    bool _c01;
    IpimBoard* _ipimBoard;
    char* _serialDevice;
    int _baselineSubtraction, _polarity;
  };
}
#endif
