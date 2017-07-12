#include "pds/ibeb/RdmaEvent.hh"
#include "pds/ibeb/RdmaRdPort.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/service/Ins.hh"

using namespace Pds::IbEb;

RdmaEvent::RdmaEvent(unsigned                  nbuff,
                     unsigned                  maxEventSize,
                     unsigned                  id,
                     std::vector<std::string>& remote) :
  RdmaBase (),
  _id      (id),
  _nbuff   (nbuff),
  _nsrc    (remote.size()),
  _pool    (new GenericPool(MAX_PUSH_SZ,nbuff*_nsrc)),
  _rpool   (new RingPool(maxEventSize*nbuff,maxEventSize)),
  _ports   (_nsrc),
  _pid       (0),
  _ncomplete (0),
  _ncorrupt  (0),
  _nbytespush(0),
  _nbytespull(0)
{
  _mr  = reg_mr( _pool->buffer(), _pool->size());
  _rmr = reg_mr(_rpool->buffer(),_rpool->size());

  for(unsigned i=0; i<_nsrc; i++) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);

    in_addr ia;
    inet_aton(remote[i].c_str(),&ia);
    sockaddr_in rsaddr;
    rsaddr.sin_family = AF_INET;
    rsaddr.sin_addr.s_addr = ia.s_addr;
    rsaddr.sin_port        = htons(RdmaPort::outlet_port);

    while (::connect(fd, (sockaddr*)&rsaddr, sizeof(rsaddr))<0) {
      perror(remote[i].c_str());
      sleep(1);
    }

    std::vector<char*> laddr(nbuff);
    for(unsigned j=0; j<nbuff; j++)
      laddr[j] = (char*)_pool->alloc(_pool->sizeofObject());

    RdmaRdPort* port = new RdmaRdPort(fd,
                                      *this,
                                      *_mr,
                                      *_rmr,
                                      laddr,
                                      id);
    _ports[i] = port;
  }

  _buff     = new uint64_t[2*nbuff];
  memset(_buff,0,2*nbuff*sizeof(uint64_t));
}

void RdmaEvent::complete(ibv_wc& wc)
{
  if (wc.opcode==IBV_WC_RECV_RDMA_WITH_IMM) {  // Push complete
    //  immediate data indicates location
    unsigned src, dst;
    RdmaRdPort::decode(wc.imm_data,src,dst);
    _ports[src]->complete(dst);
    if (_alloc(src,dst,0)) {  //  All push pieces received
      //  First stage of event filtering
      //  Now, pull big data
      for(unsigned i=0; i<_nsrc; i++) {
        const Datagram* dg = 
          reinterpret_cast<const Datagram*>(_ports[i]->push(dst));
        //  Allocate a buffer to read big data
        const SmlD& proxy = *reinterpret_cast<const SmlD*>(dg->xtc.payload());
        void* p = _rpool->alloc(proxy.extent());
        _ports[i]->req_read(dst,
                            proxy.fileOffset(),
                            p,
                            proxy.extent());
        _rpool->shrink(p,proxy.extent());
        _nbytespush += sizeof(*dg)+dg->xtc.sizeofPayload();
        _nbytespull += proxy.extent();
      }
    }
  }

  else if (wc.opcode==IBV_WC_RDMA_READ) {  // Pull complete
    unsigned src,dst;
    RdmaRdPort::decode(wc.wr_id,src,dst);
    if (_alloc(src,dst,1)) {  // All pull pieces received
      //  Count it
      _ncomplete++;
      //  Remove the data
      for(unsigned i=0; i<_nsrc; i++) {
        RingPool::free(_ports[i]->pull(dst));
        _ports[i]->ack(dst);
      }
    }
  }
}

bool RdmaEvent::_alloc(unsigned src,
                       unsigned dst,
                       unsigned stage)
{
  uint64_t& b = _buff[stage*_nbuff+dst];
  uint64_t m = (1<<_nsrc)-1;
  b |= 1ULL<<src;
  if (b==m) {
    b=0;
    return true;
  }
  else
    return false;
}
