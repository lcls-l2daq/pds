#include "pds/ibeb/RdmaBase.hh"
#include "pds/ibeb/CmpRecvPort.hh"
#include "pds/ibeb/ibcommon.hh"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace Pds::IbEb;

static bool lverbose = false;

void CmpRecvPort::verbose(bool v) { lverbose=v; }

CmpRecvPort::CmpRecvPort(int        fd,
                         RdmaBase&  base,
                         ibv_mr&    mr,
                         std::vector<char*>& laddr,
                         unsigned   idx) :
  RdmaPort(fd, base, mr, idx),
  _wr     (laddr.size()),
  _comps  (laddr)
{
  //  Work requests for receiving RDMA_WRITE_WITH_IMM
  for(unsigned i=0; i<_wr.size(); i++) {
    ibv_sge &sge = *new ibv_sge;
    memset(&sge, 0, sizeof(sge));
    sge.addr   = (uintptr_t) laddr[i];
    sge.length =  sizeof(RdmaComplete);
    sge.lkey   = mr.lkey;

    ibv_recv_wr &sr = _wr[i];
    memset(&sr,0,sizeof(sge));
    sr.next    = NULL;
    // stuff index into upper 32 bits
    sr.sg_list = &sge;
    sr.num_sge = 1;
    sr.wr_id   = i;

    ibv_recv_wr* bad_wr=NULL;
    int err = ibv_post_recv(qp(), &sr, &bad_wr);
    if (err)
      fprintf(stderr, "Failed to post SR: %s\n",  strerror(err));
    if (bad_wr)
      fprintf(stderr, "ibv_post_recv bad_wr: %lu\n", bad_wr->wr_id);
  }

  unsigned nbuff = laddr.size();
  ::write(fd, &nbuff, sizeof(unsigned));
  ::write(fd, laddr.data(), nbuff*sizeof(char*));
}

CmpRecvPort::~CmpRecvPort()
{
}

void CmpRecvPort::complete(unsigned buf)
{
  ibv_recv_wr& sr = _wr[buf];
  ibv_recv_wr* bad_wr = NULL;
  int err = ibv_post_recv(qp(), &sr, &bad_wr);
  if (err)
    fprintf(stderr, "Failed to post SR: %s\n",  strerror(err));
  if (bad_wr)
    fprintf(stderr, "ibv_post_recv bad_wr: %lu\n", bad_wr->wr_id);
}

RdmaComplete* CmpRecvPort::completion(unsigned buf) const
{
  return reinterpret_cast<RdmaComplete*>(_comps[buf]);
}
