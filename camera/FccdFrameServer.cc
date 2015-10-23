#include "pds/camera/FccdFrameServer.hh"

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

using namespace Pds;

typedef unsigned short pixel_type;

FccdFrameServer::FccdFrameServer(const Src& src) :
  FrameServer(src)
{
  _reorder_tmp = new char[FCCD_REORDER_BUFSIZE];
}

FccdFrameServer::~FccdFrameServer()
{
  delete[] _reorder_tmp;
}

#if 0
void FccdFrameServer::setFccdConfig(const FrameFccdConfigType& cfg)
{
  _config = &cfg;
  _framefwd_count = 0;
}
#endif

void FccdFrameServer::setCameraOffset(unsigned camera_offset)
{
  _camera_offset = camera_offset;
}

int FccdFrameServer::fetch(char* payload, int flags)
{
  static int err_reported = 0;
  FrameServerMsg* msg;
  int length = ::read(_fd[0],&msg,sizeof(msg));
  if (length >= 0) {
    FrameServerMsg* fmsg = msg;
    _count = fmsg->count;

    _framefwd_count = 0;  // is this OK???

    // Reorder FCCD data
    if (_reorder_frame(fmsg) != 0) {
      if (!err_reported) {
        printf("%s: _reorder_frame() error\n", __FUNCTION__);
        err_reported = 1;
      }
    }

    Xtc* xtc = new (payload) Xtc(_xtcType,_xtc.src, fmsg->damage);
    // xtc->extent += _post_fccd  (xtc->next(), fmsg);
    xtc->extent += _post_frame(xtc->next(), fmsg);
    length = xtc->extent;

    delete fmsg;
    return length;
  }
  else
    printf("FccdFrameServer::fetch error: %s\n",strerror(errno));

  return length;
}

unsigned FccdFrameServer::_post_frame(void* xtc, const FrameServerMsg* fmsg) const
{
  Frame frame(*fmsg);
  const unsigned short* frame_data = 
    reinterpret_cast<const unsigned short*>(fmsg->data);

  Xtc& frameXtc = *new((char*)xtc) Xtc(_frameType, _xtc.src, fmsg->damage);
  Frame* fp;
//if (_config->forwarding()==FrameFccdConfigType::FullFrame)
    fp=new(frameXtc.alloc(sizeof(Frame))) Frame(*fmsg,
						frame_data);
//else
//  fp=new(frameXtc.alloc(sizeof(Frame))) Frame (_config->roiBegin().column,
//  				 _config->roiEnd  ().column,
//  				 _config->roiBegin().row,
//  				 _config->roiEnd  ().row,
//  				 *fmsg);
  frameXtc.extent += fp->data_size();
  return frameXtc.extent;
}

static void forward_copy(uint32_t *dst, uint32_t *src, unsigned int count)
{
  while (count > 0) {
    *dst++ = *src++;
    count--;
  }
}

static void reverse_copy(uint32_t *dst, uint32_t *src, unsigned int count)
{
  uint32_t temp;

  dst += (count - 1);
  while (count > 0) {
    temp = *src++;
    *dst-- = (temp << 16) | (temp >> 16);   // swap 16-bit words
    count--;
  }
}

int FccdFrameServer::_reorder_frame(FrameServerMsg* fmsg)
{
  Frame frame(*fmsg);
  unsigned short* frame_data = 
    reinterpret_cast<unsigned short*>(fmsg->data);
  unsigned height = frame.height();
  unsigned width = frame.width();
#ifdef FRAME_WIDTH_16_BIT_PIXELS
  unsigned uintsPerLine = width / 2;
#else
  unsigned uintsPerLine = width / 4;
#endif
  unsigned ii;

  // sanity test
#ifdef FRAME_WIDTH_16_BIT_PIXELS
  // frame.width() expressed in 16 bit pixels
  if (!frame_data || !height || !width || (height % 2) || (width % 2) ||
      ((width * height * sizeof(uint16_t)) > FCCD_REORDER_BUFSIZE)) {
#else
  // frame.width() expressed in 8 bit pixels
  if (!frame_data || !height || !width || (height % 2) || (width % 2) ||
      ((width * height * sizeof(uint8_t)) > FCCD_REORDER_BUFSIZE)) {
#endif
    printf(">> %s ERROR: frame_data=%p height=%u width=%u FCCD_REORDER_BUFSIZE=%d\n", __FUNCTION__,
            frame_data, height, width, (unsigned) FCCD_REORDER_BUFSIZE);
    return -1;  // error
  }

  // (1) copy tap B frame_data to a temporary buffer, reversing each line

  for (ii = height / 2; ii < height; ii ++) {
    reverse_copy(((uint32_t *)_reorder_tmp) + (ii * uintsPerLine),
                 ((uint32_t *)frame_data) + (((2 * (height - ii)) - 1) * uintsPerLine),
                 uintsPerLine);
  }

  // (2) copy tap A frame_data to the top half in place
  // note: line 0 does not need to be copied

  for (ii = 1; ii < height / 2; ii ++) {
    forward_copy(((uint32_t *)frame_data) + (ii * uintsPerLine),
                 ((uint32_t *)frame_data) + (2 * ii * uintsPerLine),
                 uintsPerLine);
  }

  // (3) copy tap B frame_data from temporary buffer to the bottom half

  for (ii = height / 2; ii < height; ii ++) {
    forward_copy(((uint32_t *)frame_data) + (ii * uintsPerLine),
                 ((uint32_t *)_reorder_tmp) + (ii * uintsPerLine),
                 uintsPerLine);
  }

  return 0;
}
