#include "IpimbServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/IpimbDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

IpimbServer::IpimbServer(const Src& client, const int baselineSubtraction) :
  _xtc(_ipimbDataType,client), _baselineSubtraction(baselineSubtraction)
{
  _xtc.extent = sizeof(IpimbDataType)+sizeof(Xtc);
}

unsigned IpimbServer::offset() const {
  return sizeof(Xtc);
}

void IpimbServer::dump(int detail) const
{
}

bool IpimbServer::isValued() const
{
  return true;
}

const Src& IpimbServer::client() const
{
  return _xtc.src; 
}

const Xtc& IpimbServer::xtc() const
{
  //  unsigned* junk = (unsigned*)(&_xtc);
  //  printf("xtc: ");
  //  for (unsigned i=0;i<5;i++) {printf("%8.8x ",junk[i]);}
  //  printf("\n");
  return _xtc;
}

int IpimbServer::pend(int flag) 
{
  return -1;
}

unsigned IpimbServer::length() const {
  return _xtc.extent;
}

int IpimbServer::fetch(char* payload, int flags)
{
  IpimBoardData data = _ipimBoard->WaitData();
  if (_ipimBoard->dataDamaged()) {
    printf("IpimBoard error: IpimbServer::fetch had problems getting data, fd %d, device %s\n", fd(), _serialDevice);
    //    data.dumpRaw(); // turned off for the moment for presampling
    // need to register damage in manager (?)
  }
  //  double ch0 = data.GetCh0_V();
  unsigned long long ts = data.GetTriggerCounter();//_data->GetTimestamp_ticks();
  //  printf("data from fd %d has e.g. ch0=%fV, ts=%llu\n", fd(), ch0, ts);

  int payloadSize = sizeof(IpimbDataType);
  memcpy(payload, &_xtc, sizeof(Xtc));
  memcpy(payload+sizeof(Xtc), &data, payloadSize);
  _count = (unsigned) (0xFFFFFFFF&ts);
  return payloadSize+sizeof(Xtc);
}

int IpimbServer::fetch(ZcpFragment& zf, int flags)
{
  return 0;
}

unsigned IpimbServer::count() const
{
  return _count - 1; // "counting from" hack
}

void IpimbServer::setIpimb(IpimBoard* ipimb, char* portName) {
  _ipimBoard = ipimb;
  fd(_ipimBoard->get_fd());
  _serialDevice = portName;
}

unsigned IpimbServer::configure(IpimbConfigType& config) {
  _ipimBoard->setBaselineSubtraction(_baselineSubtraction); // avoid updating config class; caveat user
  return _ipimBoard->configure(config);
  
}

unsigned IpimbServer::unconfigure() {
  return _ipimBoard->unconfigure();
}
