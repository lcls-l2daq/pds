#include "pds/tprds/Server.hh"
#include "pds/tpr/RxDesc.hh"
#include "pds/config/Generic1DDataType.hh"
#include "pdsdata/xtc/Dgram.hh"
#include <stdio.h>

using namespace Pds;

TprDS::Server::Server(int        d,
                      const Src& src) :
  _fd  (d),
  _xtc (_generic1DDataType,src),
  _buff(new uint32_t[1024]),
  _desc(new Tpr::RxDesc(_buff,1024))
{
  fd(d);
}

TprDS::Server::~Server()
{
  delete[] _buff;
  delete   _desc;
}

int TprDS::Server::fetch(char* payload,
                         int   flags)
{
  //
  //  received data format is
  //    (4B)  length of DMA in 32-b words
  //    (2B)  Event DMA Tag (0x0000)
  //    (2B)  Channel mask (?)
  //    (4B)  Partition word (env)
  //    (8B)  PulseID
  //    (...) Payload
  //
  int nb = ::read(_fd, _desc, sizeof(*_desc));

  if ((_desc->data[1]&0xffff)==0xffff) {
    unsigned len = _desc->data[0]-2;
    for(unsigned i=0; i<len; i++)
      printf("%08x%c",_desc->data[i+2],(i&7)==7 ? '\n':' ');
    printf("--\n");
    return -1;
  }

  if (nb > 5) {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    int payloadSize = (nb-5)*sizeof(uint32_t);
    _seq = Sequence(Sequence::Event,
                    TransitionId::L1Accept,
                    ClockTime(tv),
                    TimeStamp(*reinterpret_cast<uint64_t*>(&_desc->data[3])));
    reinterpret_cast<uint32_t&>(_env) = _desc->data[2];
    _xtc.damage = 0;
    _xtc.extent = payloadSize+sizeof(Xtc);
    memcpy(payload, &_xtc, sizeof(Xtc));
    memcpy(payload+sizeof(Xtc), &_desc->data[5], payloadSize);
#ifdef DBUG
    printf("fetch: ");
    for(unsigned i=0; i<_xtc.extent>>2; i++)
      printf(" %08x",reinterpret_cast<uint32_t*>(payload)[i]);
    printf("\n");
#endif
    return _xtc.extent;
  }
  else {
    _xtc.damage.increase(Damage::UserDefined);
    _xtc.extent = sizeof(Xtc);
    memcpy(payload, &_xtc, sizeof(Xtc));
    return _xtc.extent;
  }
  return -1;
}
