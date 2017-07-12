#include "pds/ibeb/RdmaWrPort.hh"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace Pds::IbEb;

static bool lverbose = false;

void RdmaWrPort::verbose(bool v) { lverbose=v; }

RdmaWrPort::RdmaWrPort(int       fd,
                       RdmaBase& base,
                       ibv_mr&   mr,
                       unsigned  idx) :
  RdmaPort(fd, base, mr, idx),
  _wr_id  (0)
{
  if (lverbose)
    printf("RdmaSegment waiting for buffers\n");

  unsigned nbuff;
  ::read(_fd, &nbuff, sizeof(unsigned));
  _raddr.resize(nbuff);
  _laddr.resize(nbuff);
  if (lverbose)
    printf("RdmaSegment read %u buffers\n",nbuff);
  
  ::read(_fd, _raddr.data(), nbuff*sizeof(void*));
  if (lverbose)
    printf("RdmaSegment read buffer ptrs\n");
}

RdmaWrPort::~RdmaWrPort()
{
}

void     RdmaWrPort::write(unsigned src,
                           unsigned idx,
                           void*    p,
                           size_t   psize)
{
  if (lverbose) {
    printf("RdmaWrPort::write src %u  idx %u  laddr %p  raddr %llx  sz 0x%x\n",
           src, idx, p, (unsigned long long)_raddr[idx], psize);
  }

  //  launch dma
  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr   = (uintptr_t)p;
  sge.length =  psize;
  sge.lkey   = mr()->lkey;

  unsigned index = (idx<<16) | _idx;
  ibv_send_wr sr;
  memset(&sr,0,sizeof(sr));
  sr.next    = NULL;
  // stuff index into upper 32 bits
  sr.wr_id   = (uint64_t(index)<<32) | _wr_id;
  sr.sg_list = &sge;
  sr.num_sge = 1;
  sr.imm_data= (idx<<16) | src;
  sr.opcode  = IBV_WR_RDMA_WRITE_WITH_IMM;
  //      sr.send_flags = IBV_SEND_SIGNALED;
  sr.send_flags = 0;
  sr.wr.rdma.remote_addr = _raddr[idx];
  sr.wr.rdma.rkey        = _rkey;

  _laddr[idx] = (char*)p;
  _wr_id++;

  ibv_send_wr* bad_wr=NULL;
  if (ibv_post_send(qp(), &sr, &bad_wr))
    perror("Failed to post SR");
  if (bad_wr)
    perror("ibv_post_send bad_wr");
}
