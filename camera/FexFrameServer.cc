#include "pds/camera/FexFrameServer.hh"

#include "pds/camera/Frame.hh"
#include "pds/camera/TwoDGaussian.hh"

#include "pds/utility/Transition.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/camera/FrameServerMsg.hh"
#include "pds/camera/FrameType.hh"
#include "pds/camera/TwoDGaussianType.hh"
#include "pds/config/CfgCache.hh"
#include "pds/config/FrameFexConfigType.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <new>

#define DBUG

namespace Pds {
  class FexConfig : public CfgCache {
  public:
    FexConfig(const Src& src) :
      CfgCache(src,_frameFexConfigType,sizeof(FrameFexConfigType)) {}
  private:
    int _size(void* tc) const { return reinterpret_cast<FrameFexConfigType*>(tc)->size(); }
  };
};

using namespace Pds;

typedef unsigned short pixel_type;

FexFrameServer::FexFrameServer(const Src& src) :
  FrameServer  (src),
  _config      (new FexConfig (src)),
  _occPool     (new GenericPool(sizeof(UserMessage),4)),
  _hinput      ("CamInput"),
  _hfetch      ("CamFetch")
{
#ifdef DBUG
  _tinput.tv_sec = _tinput.tv_nsec = 0;
#endif
}

FexFrameServer::~FexFrameServer()
{
  delete _occPool;
  delete _config;
}

void FexFrameServer::allocate   (Transition* tr)
{
  const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
  _config ->init(alloc.allocation());
}

void FexFrameServer::doConfigure(Transition* tr)
{
  if (_config->fetch(tr) > 0) {
    _framefwd_count = 0;
  }
  else {
    printf("Config::configure failed to retrieve FrameFex configuration\n");
    _config->damage().increase(Damage::UserDefined);
  }
}

UserMessage* FexFrameServer::validate(unsigned w, unsigned h)
{
  const FrameFexConfigType& c = *reinterpret_cast<const FrameFexConfigType*>(_config->current());
  if (c.forwarding()==FrameFexConfigType::RegionOfInterest)
    if (!(c.roiBegin().column < w &&
	  c.roiBegin().row < h &&
	  c.roiEnd().column <= w &&
	  c.roiEnd().row <= h)) {
      _config->damage().increase(Damage::UserDefined);
      char buff[128];
      sprintf(buff,"FrameFexConfig ROI (col:%d-%d,row:%d-%d) exceeds frame size(col:%d,row:%d)",
	      c.roiBegin().column,c.roiEnd().column,
	      c.roiBegin().row   ,c.roiEnd().row,
	      w,h);
      return new(_occPool) UserMessage(buff);
    }
  return 0;
}

void FexFrameServer::nextConfigure(Transition* tr)
{
  _config ->next();
}

InDatagram* FexFrameServer::recordConfigure(InDatagram* in)
{
  _config ->record(in);
  return in;
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
  
  FrameServerMsg* fmsg;
  int length = ::read(_fd[0],&fmsg,sizeof(fmsg));
  if (length >= 0) {
    _count = fmsg->count;

    const FrameFexConfigType& config = *reinterpret_cast<const FrameFexConfigType*>(_config->current());
    FrameFexConfigType::Forwarding forwarding(config.forwarding());
    if (++_framefwd_count < config.forward_prescale())
      forwarding = FrameFexConfigType::NoFrame;
    else
      _framefwd_count = 0;

    Xtc* xtc = new (payload) Xtc(_xtcType,_xtc.src, fmsg->damage);

    if (config.processing() != FrameFexConfigType::NoProcessing) {
      if (forwarding == FrameFexConfigType::NoFrame)
	xtc->extent += _post_fex  (xtc->next(), fmsg);
      else {
	xtc->extent += _post_fex  (xtc->next(), fmsg);
	xtc->extent += _post_frame(xtc->next(), fmsg);
      }
    }
    else if (forwarding != FrameFexConfigType::NoFrame)
      xtc->extent += _post_frame(xtc->next(), fmsg);
    else
      ;

    length = xtc->extent;
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
  TwoDGaussian work(_feature_extract(fmsg));
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
  const FrameFexConfigType& config = *reinterpret_cast<const FrameFexConfigType*>(_config->current());
  Xtc& frameXtc = *new((char*)xtc) Xtc(_frameType, _xtc.src, fmsg->damage);
  Frame* fp;
  if (config.forwarding()==FrameFexConfigType::FullFrame)
    fp=new(frameXtc.alloc(sizeof(Frame))) Frame(*fmsg,
						fmsg->data);
  else
    fp=new(frameXtc.alloc(sizeof(Frame))) Frame (config.roiBegin().column,
						 config.roiEnd  ().column,
						 config.roiBegin().row,
						 config.roiEnd  ().row,
						 *fmsg);

  frameXtc.extent += (fp->data_size()+3)&~3;
  return frameXtc.extent;
}

TwoDMoments FexFrameServer::_feature_extract(const FrameServerMsg* msg) const
{
  //
  // perform the feature extraction here
  //
  const unsigned short* frame_data = reinterpret_cast<const unsigned short*>(msg->data);
  const FrameFexConfigType& config = *reinterpret_cast<const FrameFexConfigType*>(_config->current());
  switch(config.processing()) {
  case FrameFexConfigType::GssFullFrame:
    return TwoDMoments(msg->width, msg->height, 
		       msg->offset, frame_data);
  case FrameFexConfigType::GssRegionOfInterest:
    return TwoDMoments(msg->width,
		       config.roiBegin().column,
		       config.roiEnd  ().column,
		       config.roiBegin().row,
		       config.roiEnd  ().row,
		       msg->offset,
		       frame_data);
  case FrameFexConfigType::GssThreshold:
    return TwoDMoments(msg->width,
		       msg->height,
		       config.threshold(),
		       msg->offset,
		       frame_data);
  default:
    break;
  }
  return TwoDMoments();
}
