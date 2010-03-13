#include "pds/camera/FexFrameServer.hh"

#include "pds/camera/DmaSplice.hh"
#include "pds/camera/Frame.hh"
#include "pds/camera/FrameHandle.hh"
#include "pds/camera/TwoDGaussian.hh"

#include "pds/service/ZcpFragment.hh"
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

using namespace Pds;

typedef unsigned short pixel_type;

FexFrameServer::FexFrameServer(const Src& src, DmaSplice& splice) :
  FrameServer(src),
  _splice(splice)
{
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

    delete fmsg->handle;
    delete fmsg;
    return length;
  }
  else
    printf("FexFrameServer::fetch error: %s\n",strerror(errno));

  return length;
}

//
//  Apply feature extraction to the input frame and
//  provide the result to the (zero-copy) event builder
//
int FexFrameServer::fetch(ZcpFragment& zfo, int flags)
{
  _more = false;
  int length = 0;
  FrameServerMsg* fmsg;

  fmsg = _msg_queue.remove();
  _count = fmsg->count;

  FrameServerMsg* msg;
  length = ::read(_fd[0],&msg,sizeof(msg));
  if (length < 0) throw length;
  
  Frame frame(*fmsg->handle, _camera_offset);
  const unsigned short* frame_data = reinterpret_cast<const unsigned short*>(fmsg->handle->data);
  
  if (fmsg->type == FrameServerMsg::NewFrame) {
    
    FrameFexConfigType::Forwarding forwarding(_config->forwarding());
    if (++_framefwd_count < _config->forward_prescale())
      forwarding = FrameFexConfigType::NoFrame;
    else
      _framefwd_count = 0;

    if (_config->processing() != FrameFexConfigType::NoProcessing) {
      switch( forwarding ) {
      case FrameFexConfigType::FullFrame:
	{ Frame zcpFrame(frame.width(), frame.height(), 
			 frame.depth(), frame.offset(),
			 *fmsg->handle, _splice);
  	  length = _queue_fex_and_frame( _feature_extract(frame,frame_data), zcpFrame, fmsg, zfo );
	  break;
	}
      case FrameFexConfigType::RegionOfInterest:
	{ Frame zcpFrame(_config->roiBegin().column,
			 _config->roiEnd  ().column,
			 _config->roiBegin().row,
			 _config->roiEnd  ().row,
			 frame.width(), frame.height(), 
			 frame.depth(), frame.offset(),
			 *fmsg->handle,_splice);
  	  length = _queue_fex_and_frame( _feature_extract(frame,frame_data), zcpFrame, fmsg, zfo );
	  break;
	}
      case FrameFexConfigType::NoFrame:
	length = _queue_fex( _feature_extract(frame,frame_data), fmsg, zfo );
	break;
      }
    }
    else {
      switch (forwarding) {
      case FrameFexConfigType::FullFrame:
	{ Frame zcpFrame(frame.width(), frame.height(), 
			 frame.depth(), frame.offset(),
			 *fmsg->handle, _splice);
	  length = _queue_frame( zcpFrame, fmsg, zfo );
	  break;
	}
      case FrameFexConfigType::RegionOfInterest:
	{ Frame zcpFrame(_config->roiBegin().column,
			 _config->roiEnd  ().column,
			 _config->roiBegin().row,
			 _config->roiEnd  ().row,
			 frame.width(), frame.height(),
			 frame.depth(), frame.offset(),
			 *fmsg->handle,_splice);
	  length = _queue_frame( zcpFrame, fmsg, zfo );
	  break;
	}
      case FrameFexConfigType::NoFrame:
	length = 0;
	break;
      }
    }
  }
  else { // Post_Fragment
    _more       = true;
    _xtc.extent = fmsg->extent;
    _offset     = fmsg->offset;
    int remaining = fmsg->extent - fmsg->offset;

    try {
      if ((length = zfo.kinsert( _splice.fd(), remaining)) < 0) throw length;
      if (length != remaining) {
	fmsg->offset += length;
	fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
	::write(_fd[1],&fmsg,sizeof(fmsg));
      }
      else
	delete fmsg;
    }
    catch (int err) {
      printf("FexFrameServer::fetchz error: %s : %d\n",strerror(errno),length);
      delete fmsg;
      return -1;
    }
  }
  return length;
}

unsigned FexFrameServer::_post_fex(void* xtc, const FrameServerMsg* fmsg) const
{
  Frame frame(*fmsg->handle, _camera_offset);
  const unsigned short* frame_data = 
    reinterpret_cast<const unsigned short*>(fmsg->handle->data);

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
  Frame frame(*fmsg->handle, _camera_offset);
  const unsigned short* frame_data = 
    reinterpret_cast<const unsigned short*>(fmsg->handle->data);

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

//
//  Zero copy helper functions
//
int FexFrameServer::_queue_frame( const Frame& frame, 
				  FrameServerMsg* fmsg,
				  ZcpFragment& zfo )
{
  int frame_extent = frame.data_size();
  _xtc.contains = _frameType;
  _xtc.damage   = fmsg->damage;
  _xtc.extent = sizeof(Xtc) + sizeof(Frame) + frame_extent;

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc, sizeof(Xtc))) != sizeof(Xtc)) throw err;
    
    if ((err=zfo.uinsert( &frame, sizeof(Frame))) != sizeof(Frame)) throw err;
    
    int length;
    if ((length = zfo.kinsert( _splice.fd(), frame_extent)) < 0) throw length;
    if (length != frame_extent) { // fragmentation
      _more   = true;
      _offset = 0;
      fmsg->type   = FrameServerMsg::Fragment;
      fmsg->offset = length + sizeof(Frame) + sizeof(Xtc);
      fmsg->extent = _xtc.extent;
      fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
      ::write(_fd[1],&fmsg,sizeof(fmsg));
    }
    else
      delete fmsg;

    return length + sizeof(Frame) + sizeof(Xtc);
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    delete fmsg;
    return -1;
  }
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

int FexFrameServer::_queue_fex( const TwoDMoments& moments,
				FrameServerMsg* fmsg,
				ZcpFragment& zfo )
{	
  delete fmsg;

  _xtc.contains = _twoDGType;
  _xtc.damage   = fmsg->damage;
  _xtc.extent   = sizeof(Xtc) + sizeof(TwoDGaussianType);

  TwoDGaussian work(moments);
  TwoDGaussianType payload(work._n,
			   work._xmean,
			   work._ymean,
			   work._major_axis_width,
			   work._minor_axis_width,
			   work._major_axis_tilt);

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc, sizeof(Xtc))) < 0) throw err;
    
    if ((err=zfo.uinsert( &payload, sizeof(payload))) < 0) throw err;

    return _xtc.extent;
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    return -1;
  }
}

int FexFrameServer::_queue_fex_and_frame( const TwoDMoments& moments,
					  const Frame& frame,
					  FrameServerMsg* fmsg,
					  ZcpFragment& zfo )
{	

  Xtc fexxtc(_twoDGType, _xtc.src, fmsg->damage);
  fexxtc.extent += sizeof(TwoDGaussianType);

  Xtc frmxtc(_frameType, _xtc.src, fmsg->damage);
  frmxtc.extent += sizeof(Frame);
  frmxtc.extent += frame.data_size();

  _xtc.contains = _xtcType;
  _xtc.damage   = fmsg->damage;
  _xtc.extent   = sizeof(Xtc) + fexxtc.extent + frmxtc.extent;

  TwoDGaussian work(moments);
  TwoDGaussianType fex(work._n,
		       work._xmean,
		       work._ymean,
		       work._major_axis_width,
		       work._minor_axis_width,
		       work._major_axis_tilt);

  try {
    int err;
    if ((err=zfo.uinsert( &_xtc  , sizeof(Xtc  ))) < 0) throw err;

    if ((err=zfo.uinsert( &fexxtc, sizeof(Xtc  ))) < 0) throw err;
    if ((err=zfo.uinsert( &fex   , sizeof(fex  ))) < 0) throw err;
    
    if ((err=zfo.uinsert( &frmxtc, sizeof(Xtc  ))) < 0) throw err;
    if ((err=zfo.uinsert( &frame , sizeof(Frame))) < 0) throw err;

    int length;
    int frame_extent = frame.data_size();
    if ((length = zfo.kinsert( _splice.fd(), frame_extent)) < 0) 
      throw length;

    if (length != frame_extent) { // fragmentation
      _more   = true;
      _offset = 0;
      fmsg->type   = FrameServerMsg::Fragment;
      fmsg->offset = _xtc.extent - frame_extent + length;
      fmsg->extent = _xtc.extent;
      fmsg->connect(reinterpret_cast<FrameServerMsg*>(&_msg_queue));
      ::write(_fd[1],&fmsg,sizeof(fmsg));
    }
    else
      delete fmsg;

    return _xtc.extent + length - frame_extent;
  }
  catch (int err) {
    printf("FexFrameServer::_queue_frame error: %s : %d\n",strerror(errno),err);
    delete fmsg;
    return -1;
  }
}
