#include "TrafficDst.hh"

#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
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


ZcpTraffic::ZcpTraffic(ZcpDatagram* dg, const Ins& dst) :
  _dg(dg), _dst(dst), _iter(0)
{
}

ZcpTraffic:: ~ZcpTraffic()
{
  if (_iter) delete _iter;
  delete _dg;
}

bool ZcpTraffic::send_next(Client& client)
{
  int error;
  if (_iter) {
    error = client.send((char*)_iter->header(), 
			 _iter->payload(),
			 _iter->payloadSize(),
			 _dst);
    return _iter->next();
  }

  const Datagram& datagram = _dg->datagram();
  unsigned remaining = datagram.xtc.sizeofPayload();

  if (!remaining) {
    error = client.send((char*)&datagram,
			 (char*)&datagram.xtc,
			 sizeof(Xtc),
			 _dst);
    return false;
  }
    
  remaining -= _dg->_stream.remove(_fragment, remaining);

  if (!remaining && (datagram.xtc.extent+sizeof(Datagram)) <= Mtu::Size) {
    error = client.send((char*)&datagram,
			 (char*)&datagram.xtc,
			 sizeof(Xtc),
			 _fragment,
			 _fragment.size(),
			 _dst);
    return false;
  }
    
  _iter = new ZcpChunkIterator(_dg,_dg->_stream,_fragment);
    
  error = client.send((char*)_iter->header(),
		       (char*)&datagram.xtc,
		       sizeof(Xtc),
		       _iter->payload(),
		       _iter->payloadSize(),
		       _dst);
  return _iter->next();
}
