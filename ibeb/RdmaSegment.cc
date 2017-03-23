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
  _cpool  (0),
  _src    (index),
  //  _ndst   (numEbs),
  _wr_id  (0),
  _ports  (numEbs),
  _recvs  (numEbs)
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

  // Set up the completion ports now
  sockaddr_in csaddr;
  csaddr.sin_family = AF_INET;
  csaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  csaddr.sin_port        = htons(RdmaPort::completion_port);

  int cfd = ::socket(AF_INET, SOCK_STREAM, 0);
  int coval=1;
  ::setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &coval, sizeof(coval));
  ::bind(cfd, (sockaddr*)&csaddr, sizeof(csaddr));
  ::listen(cfd, 5);

  unsigned completionPoolSize = 0;
  unsigned nbuffs[numEbs];
  int fds[numEbs];

  // Grab buffer sizes from each of the event builders
  for(unsigned i=0; i<numEbs; i++) {
    sockaddr_in saddr;
    socklen_t   saddr_len=sizeof(saddr);

    fds[i] = ::accept(cfd, (sockaddr*)&saddr, &saddr_len);
    ::read(fds[i], &nbuffs[i], sizeof(unsigned));
    completionPoolSize += nbuffs[i];
  }

  // Once we know the buffer size allocate the completion pool and register it
  _cpool = new GenericPool(sizeof(RdmaComplete),completionPoolSize);
  _cmr = reg_mr(_cpool->buffer(),_cpool->size());

  for(unsigned i=0; i<numEbs; i++) {
    std::vector<char*> laddr(nbuffs[i]);
    for(unsigned j=0; j<nbuffs[i]; j++)
      laddr[j] = (char*)_cpool->alloc(_cpool->sizeofObject());
    //fd = ::accept(cfd, (sockaddr*)&saddr, &saddr_len);
    CmpRecvPort* recv = new CmpRecvPort(fds[i], *this, *_cmr, laddr, _src);
    _recvs[recv->idx()] = recv;
  }
  ::close(cfd);

  _elemSize = (_ports[0]->nbuff()+31)>>5;
  _buff     = new uint32_t[numEbs*_elemSize];
  memset(_buff,0,numEbs*_elemSize*sizeof(uint32_t));
}

RdmaSegment::~RdmaSegment()
{
  _pool->dump(lverbose?1:0);

  for(unsigned i=0; i<_ports.size(); i++)
    delete _ports[i];

  for(unsigned i=0; i<_recvs.size(); i++)
    delete _recvs[i];

  ibv_dereg_mr(_mr);
  ibv_dereg_mr(_cmr);

  delete _pool;
  delete _cpool;
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

void RdmaSegment::complete(ibv_wc& wc, Semaphore& sem)
{
  if (wc.opcode==IBV_WC_RECV_RDMA_WITH_IMM) {
    unsigned dst, dstIdx;
    CmpRecvPort::decode(wc.imm_data,dst,dstIdx);
    sem.take();
    dequeue(*_recvs[dst]->completion(dstIdx));
    _recvs[dst]->complete(dstIdx);
    sem.give();
  }
}
