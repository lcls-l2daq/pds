#include "TrafficDst.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/utility/ChunkIterator.hh"
#include "pds/service/Client.hh"

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


