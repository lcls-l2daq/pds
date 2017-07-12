#include "pds/ibeb/RdmaSegment.hh"

#include "pds/xtc/Datagram.hh"

#include <unistd.h>

using namespace Pds::IbEb;

static bool lverbose = false;

void RdmaSegment::verbose(bool v) { lverbose=v; }

RdmaSegment::RdmaSegment(unsigned   poolSize,
                         unsigned   maxEventSize,
                         unsigned   index,
                         unsigned   numEbs) :
  RdmaBase(),
  _pool   (new RingPool(poolSize,maxEventSize)),
  _src    (index),
  //  _ndst   (numEbs),
  _wr_id  (0),
  _ports  (numEbs)
{
  _mr = reg_mr(_pool->buffer(),_pool->size());

  sockaddr_in rsaddr;
  rsaddr.sin_family = AF_INET;
  rsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  rsaddr.sin_port        = htons(RdmaPort::outlet_port);

  int lfd = ::socket(AF_INET, SOCK_STREAM, 0);
  int oval=1;
  ::setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &oval, sizeof(oval));
  ::bind(lfd, (sockaddr*)&rsaddr, sizeof(rsaddr));
  ::listen(lfd, 5);
  for(unsigned i=0; i<numEbs; i++) {
    sockaddr_in saddr;
    socklen_t   saddr_len=sizeof(saddr);
    int fd = ::accept(lfd, (sockaddr*)&saddr, &saddr_len);
    RdmaWrPort* port = new RdmaWrPort(fd, *this, *_mr, _src);
    _ports[port->idx()] = port;
  }
  ::close(lfd);

  _elemSize = (_ports[0]->nbuff()+31)>>5;
  _buff     = new uint32_t[numEbs*_elemSize];
  memset(_buff,0,numEbs*_elemSize*sizeof(uint32_t));
}

RdmaSegment::~RdmaSegment()
{
  _pool->dump(lverbose?1:0);

  for(unsigned i=0; i<_ports.size(); i++)
    delete _ports[i];

  ibv_dereg_mr(_mr);

  delete _pool;
  delete[] _buff;
}

unsigned RdmaSegment::nBuffers() const 
{
  return _ports[0]->nbuff();
}

void RdmaSegment::queue(unsigned  dst, 
                        unsigned  tgt, 
                        Datagram* p, 
                        size_t    psize)
{
  _pool->shrink(p,psize);
  if (lverbose) {
    printf("Queue %p\n",p);
    _pool->dump(0);
  }
  if (_alloc(dst,tgt)) {
    //  Launch RDMA_WR
    req_write(dst,tgt,p);
  }
  else {
    // queue
    _dqueue.push_back(dst);
    _tqueue.push_back(tgt);
    _pqueue.push_back(p);
  }
}

void RdmaSegment::dequeue(const RdmaComplete& cmpl)
{
  void* p = _ports[cmpl.dst]->laddr(cmpl.dstIdx);
  if (lverbose) {
    printf("Dequeue %p\n",p);
    _pool->dump(0);
  }

  RingPool::free(p);
  _dealloc(cmpl.dst,cmpl.dstIdx);

  while(!_dqueue.empty() && _alloc(_dqueue.front(),_tqueue.front())) {
    //  Launch RDMA_WR
    req_write(_dqueue.front(),_tqueue.front(),_pqueue.front());
    //  Pop off queue
    _dqueue.pop_front();
    _tqueue.pop_front();
    _pqueue.pop_front();
  }
}

bool RdmaSegment::_alloc(unsigned eb, 
                         unsigned buf) 
{
  uint32_t& b = _buff[eb*_elemSize + (buf>>5)];
  unsigned m = 1<<(buf&0x1f);
  if (b&m)
    return false;
  else
    b |= m;
  if (lverbose)
    printf("alloc _buff[%u]=%08x\n",
           eb*_elemSize+(buf>>5),
           b);
  return true;
}

void RdmaSegment::_dealloc(unsigned eb, 
                           unsigned buf) 
{
  uint32_t& b = _buff[eb*_elemSize + (buf>>5)];
  unsigned m = 1<<(buf&0x1f);
  b &= ~m;
  if (lverbose)
    printf("dealloc _buff[%u]=%08x\n",
           eb*_elemSize+(buf>>5),
           b);
}

void RdmaSegment::req_write(unsigned  eb,
                            unsigned  index, 
                            Datagram* dg)
{
  _ports[eb]->write(_src, index, (void*)dg, 
                    sizeof(*dg)+dg->xtc.sizeofPayload());
}
