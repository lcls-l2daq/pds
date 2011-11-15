#include "pds/camera/FexFrameServer.hh"

#include "pds/camera/Frame.hh"
#include "pds/camera/TwoDGaussian.hh"

#include "pds/xtc/XtcType.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/camera/FrameType.hh"
#include "pds/camera/TwoDGaussianType.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <new>

#define DBUG

using namespace Pds;

typedef unsigned short pixel_type;

FexFrameServer::FexFrameServer(const Src& src) :
  FrameServer(src),
  _hinput      ("CamInput"),
  _hfetch      ("CamFetch")
{
#ifdef DBUG
  _tinput.tv_sec = _tinput.tv_nsec = 0;
#endif
}

FexFrameServer::~FexFrameServer()
{
}

void FexFrameServer::setFexConfig(const FrameFexConfigType& cfg)
{
  _config = &cfg;
  _framefwd_count = 0;
}

void FexFrameServer::setCameraOffset(unsigned camera_offset)
{
  _camera_offset = camera_offset;
}

//
//  Apply feature extraction to the input frame and
//  provide the result to the event builder
//
int FexFrameServer::fetch(char* payload, int flags)
{
#ifdef DBUG
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  if (_tinput.tv_sec)
    _hinput.accumulate(ts,_tinput);
  _tinput=ts;
#endif
  
  FrameServerMsg* msg;
  int length = ::read(_fd[0],&msg,sizeof(msg));
  if (length >= 0) {
    FrameServerMsg* fmsg = _msg_queue.remove();
    _count = fmsg->count;

    //  Is pipe write/read good enough?
    if (msg != fmsg) printf("Overlapping events %d/%d\n",msg->count,fmsg->count);

    FrameFexConfigType::Forwarding forwarding(_config->forwarding());
    if (++_framefwd_count < _config->forward_prescale())
      forwarding = FrameFexConfigType::NoFrame;
    else
      _framefwd_count = 0;

    if (_config->processing() != FrameFexConfigType::NoProcessing) {
      if (forwarding == FrameFexConfigType::NoFrame)
	length = _post_fex( payload, fmsg);
      else {
	Xtc* xtc = new (payload) Xtc(_xtcType,_xtc.src, fmsg->damage);
	xtc->extent += _post_fex  (xtc->next(), fmsg);
	xtc->extent += _post_frame(xtc->next(), fmsg);
	length = xtc->extent;
      }
    }
    else if (forwarding != FrameFexConfigType::NoFrame)
      length = _post_frame( payload, fmsg);
    else
      length = 0;

    _xtc.damage = fmsg->damage;

    delete fmsg;
  }
  else
    printf("FexFrameServer::fetch error: %s\n",strerror(errno));

#ifdef DBUG
  clock_gettime(CLOCK_REALTIME, &ts);
  _hfetch.accumulate(ts,_tinput);

  _hinput.print(_count+1);
  _hfetch.print(_count+1);
#endif
  return length;
}

//
//  Apply feature extraction to the input frame and
//  provide the result to the (zero-copy) event builder
//
int FexFrameServer::fetch(ZcpFragment& zfo, int flags)
{
  return -1;
}

unsigned FexFrameServer::_post_fex(void* xtc, const FrameServerMsg* fmsg) const
{
  Frame frame(fmsg->width, fmsg->height, fmsg->depth, _camera_offset);
  const unsigned short* frame_data = 
    reinterpret_cast<const unsigned short*>(fmsg->data);

  TwoDGaussian work(_feature_extract(frame,frame_data));
  Xtc& fexXtc = *new((char*)xtc) Xtc(_twoDGType, _xtc.src, fmsg->damage);
  new(fexXtc.alloc(sizeof(TwoDGaussianType))) TwoDGaussianType(work._n,
							       work._xmean,
							       work._ymean,
							       work._major_axis_width,
							       work._minor_axis_width,
							       work._major_axis_tilt);
  return fexXtc.extent;
}

unsigned FexFrameServer::_post_frame(void* xtc, const FrameServerMsg* fmsg) const
{
  Frame frame(fmsg->width, fmsg->height, fmsg->depth, _camera_offset);
  const unsigned short* frame_data = 
    reinterpret_cast<const unsigned short*>(fmsg->data);

  Xtc& frameXtc = *new((char*)xtc) Xtc(_frameType, _xtc.src, fmsg->damage);
  Frame* fp;
  if (_config->forwarding()==FrameFexConfigType::FullFrame)
    fp=new(frameXtc.alloc(sizeof(Frame))) Frame(frame.width(), frame.height(), 
						frame.depth(), frame.offset(),
						frame_data);
  else
    fp=new(frameXtc.alloc(sizeof(Frame))) Frame (_config->roiBegin().column,
						 _config->roiEnd  ().column,
						 _config->roiBegin().row,
						 _config->roiEnd  ().row,
						 frame.width(), frame.height(), 
						 frame.depth(), frame.offset(),
						 frame_data);
  frameXtc.extent += fp->data_size();
  return frameXtc.extent;
}

TwoDMoments FexFrameServer::_feature_extract(const Frame&          frame,
					     const unsigned short* frame_data) const
{
  //
  // perform the feature extraction here
  //
  switch(_config->processing()) {
  case FrameFexConfigType::GssFullFrame:
    return TwoDMoments(frame.width(), frame.height(), 
		       frame.offset(), frame_data);
  case FrameFexConfigType::GssRegionOfInterest:
    return TwoDMoments(frame.width(),
		       _config->roiBegin().column,
		       _config->roiEnd  ().column,
		       _config->roiBegin().row,
		       _config->roiEnd  ().row,
		       frame.offset(),
		       frame_data);
  case FrameFexConfigType::GssThreshold:
    return TwoDMoments(frame.width(),
		       frame.height(),
		       _config->threshold(),
		       frame.offset(),
		       frame_data);
  default:
    break;
  }
  return TwoDMoments();
}
