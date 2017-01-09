#include "pds/ibeb/RdmaBase.hh"
#include "pds/ibeb/RdmaRdPort.hh"
#include "pds/ibeb/ibcommon.hh"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace Pds::IbEb;

static bool lverbose = false;

void RdmaRdPort::verbose(bool v) { lverbose=v; }

RdmaRdPort::RdmaRdPort(int       fd,
                       RdmaBase& base,
                       ibv_mr&   mr,
                       ibv_mr&   rmr,
                       std::vector<char*>& laddr,
                       unsigned  idx) :
  RdmaPort(fd, base, mr, idx),
  _wr     (laddr.size()),
  _swr    (laddr.size()),
  _push   (laddr),
  _pull   (laddr.size()),
  _rpull  (laddr.size()),
  _dst    (idx),
  _swrid  (0)
{
  //  Work requests for receiving RDMA_WRITE_WITH_IMM
  for(unsigned i=0; i<_wr.size(); i++) {
    ibv_sge &sge = *new ibv_sge;
    memset(&sge, 0, sizeof(sge));
    sge.addr   = (uintptr_t)new unsigned;
    sge.length =  sizeof(unsigned);
    sge.lkey   = mr.lkey;
        
    ibv_recv_wr &sr = _wr[i];
    memset(&sr,0,sizeof(sr));
    sr.next    = NULL;
    // stuff index into upper 32 bits
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.wr_id   = i;
        
    ibv_recv_wr* bad_wr=NULL;
    if (ibv_post_recv(qp(), &sr, &bad_wr))
      perror("Failed to post SR");
    if (bad_wr)
      perror("ibv_post_recv bad_wr");
  }    

  //  Work requests for RDMA_READ
  for(unsigned i=0; i<_swr.size(); i++) {
    ibv_sge &sge = *new ibv_sge;
    memset(&sge, 0, sizeof(sge));
    sge.lkey     = rmr.lkey;
        
    ibv_send_wr &sr = _swr[i];
    memset(&sr,0,sizeof(sr));
    sr.next       = NULL;
    sr.opcode     = IBV_WR_RDMA_READ;
    sr.send_flags = IBV_SEND_SIGNALED;
    // sr.send_flags = 0;
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.wr.rdma.rkey = _rkey;
  }    

  unsigned nbuff = laddr.size();
  ::write(fd, &nbuff, sizeof(unsigned));
  ::write(fd, laddr.data(), nbuff*sizeof(char*));
}

RdmaRdPort::~RdmaRdPort()
{
}

void RdmaRdPort::req_read(unsigned buf,
                          uint64_t raddr,
                          void*    p,
                          unsigned psize)
{
  if (lverbose) {
    printf("RdmaRdPort::req_read buf %u  raddr %llx  laddr %p  sz 0x%x\n",
           buf, (unsigned long long)raddr, p, psize);
  }

  _pull [buf] = reinterpret_cast<char*>(p);
  _rpull[buf] = reinterpret_cast<char*>(raddr);

  //  launch dma
  ibv_send_wr& sr = _swr[buf];
  ibv_sge& sge = *sr.sg_list;
  sge.addr   = (uintptr_t)p;
  sge.length =  psize;

  unsigned code = _encode(buf);
  sr.wr_id    = (uint64_t(code)<<32) | _swrid;
  sr.imm_data = code;
  sr.wr.rdma.remote_addr = raddr;
  
  _swrid++;
  
  ibv_send_wr* bad_wr=NULL;
  if (ibv_post_send(qp(), &sr, &bad_wr))
    perror("Failed to post SR (read)");
  if (bad_wr)
    perror("ibv_post_send bad_wr (read)");
}

void RdmaRdPort::complete(unsigned buf)
{
  ibv_recv_wr& sr = _wr[buf];
  ibv_recv_wr* bad_wr=NULL;
  if (ibv_post_recv(qp(), &sr, &bad_wr))
    perror("Failed to post SR");
  if (bad_wr)
    perror("ibv_post_recv bad_wr");
}

void RdmaRdPort::ack(unsigned buf)
{
  if (lverbose)
    printf("RdmaRdPort::ack  buf %u  dst %u  dg %p\n",
           buf, _dst, _rpull[buf]);

  //  Acknowledge reception
  RdmaComplete cmpl;
  cmpl.dst    = _dst;
  cmpl.dstIdx = buf;
  cmpl.dg     = _rpull[buf];
  ::write(fd(),&cmpl,sizeof(cmpl));
}

