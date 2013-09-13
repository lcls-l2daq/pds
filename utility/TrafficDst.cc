#include "TrafficDst.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/utility/ChunkIterator.hh"
#include "pds/service/Client.hh"

namespace Pds {
  class VirtualTraffic : public TrafficDst {
  public:
    VirtualTraffic(CDatagram*);
    ~VirtualTraffic();
  public:
    bool send_next(Client&);
    void send_copy(Client&,const Ins&) {}
  public:
    TrafficDst* clone() const;
  private:
    CDatagram*       _dg;
    Ins              _dst;
    DgChunkIterator* _iter;
  };
};

using namespace Pds;

CTraffic::CTraffic(CDatagram* dg, const Ins& dst) :
  _dg(dg), _dst(dst)
{
  const Datagram& datagram = _dg->datagram();
  unsigned size = datagram.xtc.extent;

  if(size > Mtu::Size) 
    _iter = new DgChunkIterator(&dg->datagram());
  else
    _iter = 0;
}

CTraffic::~CTraffic() 
{
  if (_iter) delete _iter; 
  delete _dg; 
}

bool CTraffic::send_next(Client& client)
{
  if (_iter) {
    int error;
    if((error = client.send((char*)_iter->header(),
			    (char*)_iter->payload(),
			    _iter->payloadSize(),
			    _dst)))
      ;
    return _iter->next();
  }
  else {
    const Datagram& datagram = _dg->datagram();
    unsigned size = datagram.xtc.extent;
    int error;
    if ((error = client.send((char*)&datagram,
			     (char*)&datagram.xtc,
			     size,
			     _dst)))
      ;
    return false;
  }
}

void CTraffic::send_copy(Client& client, const Ins& dst)
{
  if (_iter) {
    int error;
    if((error = client.send((char*)_iter->header(),
			    (char*)_iter->payload(),
			    _iter->payloadSize(),
			    dst)))
      ;
  }
  else {
    const Datagram& datagram = _dg->datagram();
    unsigned size = datagram.xtc.extent;
    int error;
    if ((error = client.send((char*)&datagram,
			     (char*)&datagram.xtc,
			     size,
			     dst)))
      ;
  }
}

TrafficDst* CTraffic::clone() const
{
  return new VirtualTraffic(_dg);
}

#include "pds/utility/StreamPorts.hh"

VirtualTraffic::VirtualTraffic(CDatagram* dg) :
  _dg(dg), _dst(StreamPorts::sink())
{
  const Datagram& datagram = _dg->datagram();
  unsigned size = datagram.xtc.extent;

  if(size > Mtu::Size) 
    _iter = new DgChunkIterator(&dg->datagram());
  else
    _iter = 0;
}

VirtualTraffic::~VirtualTraffic() 
{
  if (_iter) delete _iter; 
}

bool VirtualTraffic::send_next(Client& client)
{
  if (_iter) {
    int error;
    if((error = client.send((char*)_iter->header(),
			    (char*)_iter->payload(),
			    _iter->payloadSize(),
			    _dst)))
      ;
    return _iter->next();
  }
  else {
    const Datagram& datagram = _dg->datagram();
    unsigned size = datagram.xtc.extent;
    int error;
    if ((error = client.send((char*)&datagram,
			     (char*)&datagram.xtc,
			     size,
			     _dst)))
      ;
    return false;
  }
}

TrafficDst* VirtualTraffic::clone() const
{
  return new VirtualTraffic(_dg);
}
