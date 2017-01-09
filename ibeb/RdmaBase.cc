#include "pds/ibeb/RdmaBase.hh"

#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

using namespace Pds::IbEb;

RdmaBase::RdmaBase() :
  _ctx(0),
  _pd (0),
  _cq (0)
{
  {
    int num_devices=0;
    ibv_device** dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list) {
      printf("Failed to get IB devices list\n");
      return;
    }
  
    printf("Found %d devices\n", num_devices);
    if (!num_devices)
      return;
    
    _ctx = ibv_open_device(dev_list[0]);
    ibv_free_device_list(dev_list);
  }

  //  Query port properties
  static const int ib_port=1;
  ibv_port_attr   port_attr;
  if (ibv_query_port(_ctx, ib_port, &port_attr)) {
    perror("ibv_query_port failed");
    return;
  }

  //  Allocate Protection Domain
  if ((_pd = ibv_alloc_pd(_ctx))==0) {
    perror("ibv_alloc_pd failed");
    abort();
  }
  
  // if ((_cc = ibv_create_comp_channel(_ctx)) == 0) {
  //   perror("Failed to create CC");
  //   abort();
  // }

  // if ((_cq = ibv_create_cq(_ctx, 16, NULL, _cc, 0))==0) {
  if ((_cq = ibv_create_cq(_ctx, 256, NULL, NULL, 0))==0) {
    perror("Failed to create CQ");
    abort();
  }
}

RdmaBase::~RdmaBase()
{
  ibv_destroy_cq(_cq);
  //  ibv_destroy_comp_channel(_cc);
  ibv_dealloc_pd(_pd);
  ibv_close_device(_ctx);
}

ibv_mr* RdmaBase::reg_mr(void* p, size_t memsize)
{
  //  Register the memory buffers
  int mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

  ibv_mr* mr;

  if ((mr = ibv_reg_mr(_pd, p, memsize, mr_flags))==0) {
    perror("ibv_reg_mr failed");
    abort();
  }

  printf("MR[%zu] was registered with addr %p, lkey %x, rkey %x, flags %x\n",
         sizeof(*mr), p, mr->lkey, mr->rkey, mr_flags);

  return mr;
}
