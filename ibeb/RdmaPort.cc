#include "pds/ibeb/RdmaPort.hh"
#include "pds/ibeb/RdmaBase.hh"
#include "pds/ibeb/ibcommon.hh"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

using namespace Pds::IbEb;

static const int ib_port=1;

RdmaPort::RdmaPort(int       fd,
                   RdmaBase& base,
                   ibv_mr&   mr,
                   unsigned  idx) :
  _fd(fd),
  _qp(0),
  _mr(mr),
  _rkey(0),
  _idx (-1)
{
  //  Query port properties
  ibv_port_attr   port_attr;
  if (ibv_query_port(base.ctx(), ib_port, &port_attr)) {
    perror("ibv_query_port failed");
    return;
  }
  
  //  Create the QP
  ibv_qp_init_attr qp_init_attr;
  memset(&qp_init_attr,0,sizeof(qp_init_attr));

  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.sq_sig_all = 1;
  qp_init_attr.send_cq = base.cq();
  qp_init_attr.recv_cq = base.cq();
  qp_init_attr.cap.max_send_wr = 32;
  qp_init_attr.cap.max_recv_wr = 32;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;
  qp_init_attr.cap.max_inline_data = 32*sizeof(unsigned);

  if ((_qp = ibv_create_qp(base.pd(),&qp_init_attr))==0) {
    perror("Failed to create QP");
    abort();
  }

  printf("QP was created, QP number %x\n",
         _qp->qp_num);

  ibv_gid gid;
  //  ibv_query_gid(_ctx, ib_port, 0, &gid);
  memset(&gid,0,sizeof(gid));

  IbEndPt endpt;
  endpt.rkey   = _mr.rkey;
  endpt.qp_num = _qp->qp_num;
  endpt.lid    = port_attr.lid;
  memcpy(&endpt.gid, &gid, 16);
  endpt.idx    = idx;

  printf("Local LID %x\n",port_attr.lid);

  ::write(_fd, &endpt, sizeof(endpt));
  { int bytes=0, remaining=sizeof(endpt);
    char* p = reinterpret_cast<char*>(&endpt);
    while(remaining) {
      bytes = ::read(_fd, p, remaining);
      remaining -= bytes;
      p += bytes;
    }
  }

  //  printf("Remote addr %lx\n",endpt.addr);
  printf("Remote rkey %x\n", endpt.rkey);
  printf("Remote QP number %x\n", endpt.qp_num);
  printf("Remote LID %x\n", endpt.lid);
    
  printf("Rdma setup with  lkey[%u] %x  rkey[%u] %x\n",
         idx, _mr.rkey, endpt.idx, endpt.rkey);

  _rkey = endpt.rkey;
  _idx  = endpt.idx;

  {
    ibv_qp_attr attr;

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = ib_port;
    attr.pkey_index = 0;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | 
      IBV_ACCESS_REMOTE_READ |
      IBV_ACCESS_REMOTE_WRITE;

    int flags = IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS;

    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QT state to INIT");
      abort();
    }
  }

  {
    ibv_qp_attr attr;
    memset(&attr,0,sizeof(attr));
    
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_256;
    attr.dest_qp_num = endpt.qp_num;
    attr.rq_psn      = 0;
    attr.max_dest_rd_atomic=1;
    attr.min_rnr_timer=0x12;
    attr.ah_attr.is_global=0;
    attr.ah_attr.dlid= endpt.lid;
    attr.ah_attr.sl  = 0;
    attr.ah_attr.src_path_bits=0;
    attr.ah_attr.port_num = ib_port;
    
    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
      IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
      IBV_QP_MIN_RNR_TIMER;
    
    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QP state to RTR");
      abort();
    }
  }
 
  {
    ibv_qp_attr attr;

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 0x12;
    attr.retry_cnt = 6;
    attr.rnr_retry=0;
    attr.sq_psn = 0;
    attr.max_rd_atomic=1;
    
    int flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
      IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;
    
    int rc = ibv_modify_qp(_qp,&attr,flags);
    if (rc) {
      perror("Failed to modify QP state to RTS");
      abort();
    }
  }
}
  
RdmaPort::~RdmaPort()
{
  ibv_destroy_qp(_qp);
}
