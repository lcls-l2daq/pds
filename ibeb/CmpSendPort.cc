#include "pds/ibeb/CmpSendPort.hh"
#include "pds/ibeb/ibcommon.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace Pds::IbEb;

static bool lverbose = false;

void CmpSendPort::verbose(bool v) { lverbose=v; }

CmpSendPort::CmpSendPort(int       fd,
                         RdmaBase& base,
                         ibv_mr&   mr,
                         std::vector<void*> laddr,
                         unsigned  idx) :
  RdmaPort(fd, base, mr, idx, laddr.size(), sizeof(RdmaComplete)),
  _wr_id  (0),
  _src    (idx),
  _laddr  (laddr)
{
  unsigned nbuff = 0;
  if (lverbose)
    printf("CmpSendPort waiting for buffers\n");
  ::read(_fd, &nbuff, sizeof(unsigned));
  _raddr.resize(nbuff);
  if (_laddr.size() != nbuff) {
    printf("CmpSendPort local addr buffer size %lu) differs from remote: %u\n", laddr.size(), nbuff);
    abort();
  }
  if (lverbose)
    printf("CmpSendPort read %u buffers\n",nbuff);

  ::read(_fd, _raddr.data(), nbuff*sizeof(void*));
  if (lverbose)
    printf("CmpSendPort read buffer ptrs\n");
}

CmpSendPort::~CmpSendPort()
{
}

void CmpSendPort::ack(unsigned buf, char* dg)
{
  if (lverbose)
    printf("CmpSendPort::ack  buf %u  dst %u  dg %p\n",
            buf, _idx, dg);

  unsigned index = encode(_src, buf);
  RdmaComplete* cmp = reinterpret_cast<RdmaComplete*>(_laddr[buf]);
  cmp->dst    = _src;
  cmp->dstIdx = buf;
  cmp->dg     = dg;

  //  launch dma
  ibv_sge sge;
  memset(&sge, 0, sizeof(sge));
  sge.addr   = (uintptr_t) _laddr[buf];
  sge.length =  sizeof(RdmaComplete);
  sge.lkey   = mr()->lkey;

  ibv_send_wr sr;
  memset(&sr,0,sizeof(sr));
  sr.next    = NULL;
  // stuff index into upper 32 bits
  sr.wr_id   = (uint64_t(index)<<32) | _wr_id;
  sr.sg_list = &sge;
  sr.num_sge = 1;
  sr.imm_data= index;
  sr.opcode  = IBV_WR_RDMA_WRITE_WITH_IMM;
  //      sr.send_flags = IBV_SEND_SIGNALED;
  sr.send_flags = IBV_SEND_INLINE;
  sr.wr.rdma.remote_addr = _raddr[buf];
  sr.wr.rdma.rkey        = _rkey;

  _wr_id++;

  ibv_send_wr* bad_wr=NULL;
  if (ibv_post_send(qp(), &sr, &bad_wr))
    perror("Failed to post SR");
  if (bad_wr)
    perror("ibv_post_send bad_wr");
}
