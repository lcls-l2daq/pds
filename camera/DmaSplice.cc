#include "DmaSplice.hh"

#include "pds/camera/LvCamera.hh"
#include "pds/camera/FrameHandle.hh"
#include "pds/service/Task.hh"
#include "pds/zerocopy/dma_splice/dma_splice.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

DmaSplice::DmaSplice()
{
  _fd = dma_splice_open();
  if (_fd < 0)
    printf("Error opening dma_splice: %s\n",strerror(errno));

  _task = new Task(TaskObject("DmaSplice_release"));
  _task->call(this);
}

DmaSplice::~DmaSplice()
{
  _task->destroy();
  close(_fd);
}

void DmaSplice::initialize(void* base, unsigned len)
{
  dma_splice_initialize(_fd, base, len);
}

void DmaSplice::queue(void* data, unsigned framesize, unsigned rel_arg)
{
  dma_splice_queue(_fd, data, framesize, rel_arg);
}

void DmaSplice::routine(void)
{
  unsigned long release_arg;
  int ret;
  while(1) {
    if (!(ret=dma_splice_notify(_fd, &release_arg)) ) {
      PdsLeutron::FrameHandle* pFrame = 
	reinterpret_cast<PdsLeutron::FrameHandle*>(release_arg);
      delete pFrame;
    }
    else
      printf("DmaSplice::notify failed %s\n",strerror(-ret));
  }
}
