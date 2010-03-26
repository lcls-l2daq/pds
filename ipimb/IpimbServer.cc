#include "IpimbServer.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/IpimbDataType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

IpimbServer::IpimbServer(const Src& client) :
  _xtc(_ipimbDataType,client)
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
  bool success = false;
  int timeout_us = 10000; // 10 ms - ???
  //  printf("wait for data in IS:fetch, fd %d, payload pointer %p this call\n", fd(), payload);
  IpimBoardData data = _ipimBoard->WaitData(timeout_us, success);
  if (!success) {
    printf("IpimBoard error: fetch data had problems, presumably timed out\n");
    // register damage?
  }
  //  double ch0 = data.GetCh0_V();
  unsigned long long ts = data.GetTriggerCounter();//_data->GetTimestamp_ticks();
  //  printf("data from fd %d has e.g. ch0=%fV, ts=%llu\n", fd(), ch0, ts);
  //  data.dumpRaw();

  int payloadSize = sizeof(IpimbDataType);
  memcpy(payload, &_xtc, sizeof(Xtc));
  memcpy(payload+sizeof(Xtc), &data, payloadSize);
  _count = (unsigned) (0xFFFFFFFF&ts) - _fakeCount;
  return payloadSize+sizeof(Xtc);
}

int IpimbServer::fetch(ZcpFragment& zf, int flags)
{
  return 0;
}

unsigned IpimbServer::count() const
{
  if (_count%1000 == 0) {
    printf("in IpimbServer::count, fd %d: returning %d\n", fd(), _count-1);
  }
  return _count - 1;
}

void IpimbServer::setIpimb(IpimBoard* ipimb) {
  _ipimBoard = ipimb;
  fd(_ipimBoard->get_fd());
}

void IpimbServer::setFakeCount(unsigned fakeCount) {
  _fakeCount = fakeCount;
}

unsigned IpimbServer::configure(const IpimbConfigType& config) {
  return _ipimBoard->configure(config);
}

unsigned IpimbServer::GetTriggerCounter1() {
  return _ipimBoard->GetTriggerCounter1();
}
