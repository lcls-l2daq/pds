#ifndef TrafficDst_hh
#define TrafficDst_hh

#include "pds/service/Ins.hh"
#include "pds/service/LinkedList.hh"
#include "pds/service/ZcpFragment.hh"

namespace Pds {
  class Client;
  class DgChunkIterator;
  class ZcpChunkIterator;
  class CDatagram;
  class ZcpDatagram;

  class TrafficDst : public LinkedList<TrafficDst> {
  public:
    virtual ~TrafficDst() {}
    virtual bool send_next(Client&) = 0;
  };

  class CTraffic : public TrafficDst {
  public:
    CTraffic(CDatagram*, const Ins&);
    ~CTraffic();
  public:
    bool send_next(Client&);
  private:
    CDatagram*       _dg;
    Ins              _dst;
    DgChunkIterator* _iter;
  };

  class ZcpTraffic : public TrafficDst {
  public:
    ZcpTraffic(ZcpDatagram*, const Ins&);
    ~ZcpTraffic();
  public:
    bool send_next(Client&);
  private:
    ZcpDatagram*      _dg;
    Ins               _dst;
    ZcpFragment       _fragment;
    ZcpChunkIterator* _iter;
  };
};

#endif
