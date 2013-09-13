#ifndef TrafficDst_hh
#define TrafficDst_hh

#include "pds/service/Ins.hh"
#include "pds/service/LinkedList.hh"

namespace Pds {
  class Client;
  class DgChunkIterator;
  class CDatagram;

  class TrafficDst : public LinkedList<TrafficDst> {
  public:
    virtual ~TrafficDst() {}
    virtual bool send_next(Client&) = 0;
    virtual void send_copy(Client&,const Ins&) = 0;
    virtual TrafficDst* clone() const = 0;
  };

  class CTraffic : public TrafficDst {
  public:
    CTraffic(CDatagram*, const Ins&);
    ~CTraffic();
  public:
    bool send_next(Client&);
    void send_copy(Client&,const Ins&);
  public:
    TrafficDst* clone() const;
  private:
    CDatagram*       _dg;
    Ins              _dst;
    DgChunkIterator* _iter;
  };
};

#endif
