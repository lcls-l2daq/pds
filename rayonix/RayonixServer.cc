#include "RayonixServer.hh"
#include "pds/xtc/XtcType.hh"
#include "pds/camera/FrameType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>

using namespace Pds;

#define MAXFRAME             (32 * 1024 * 1024)

Pds::RayonixServer::RayonixServer( const Src& client, bool verbose)
  : _xtc        ( _frameType, client ),
    _xtcDamaged ( _frameType, client ),
    _occSend(NULL),
    _verbose(verbose),
    _binning_f(0),
    _binning_s(0),
    _darkFlag(0),
    _readoutMode(1),
    _trigger(0),
    _rnxctrl(NULL),
    _rnxdata(NULL)
{
  // xtc extent in case of damaged event
  _xtcDamaged.extent = sizeof(Xtc);
  _xtcDamaged.damage.increase(Pds::Damage::UserDefined);

  // set up to read data from socket
  _rnxdata = new Pds::rayonix_data::rayonix_data(_verbose);
  fd(_rnxdata->fd());
  _rnxdata->reset(_verbose);
}

unsigned Pds::RayonixServer::configure(RayonixConfigType& config)
{
  unsigned numErrs = 1;
  int status;
  int binning_f = (int)config.binning_f();
  int binning_s = (int)config.binning_s();
  int exposure = (int)config.exposure();
  int trigger = (int)config.trigger();
  int rawMode = (int)config.rawMode();
  int darkFlag = (int)config.darkFlag();
  int readoutMode = (int)config.readoutMode();    
  char deviceBuf[Pds::rayonix_control::DeviceIDMax+1];
  //  int testPattern = (int) config.testPattern();
  int testPattern = 0;
  char msgBuf[100];
  
  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_rnxctrl) {
    printf("ERROR: _rnxctrl is not NULL at beginning of %s\n", __PRETTY_FUNCTION__);
  }

  // If the binning is not the same in both fast and slow directions, send an occurrence
  if (_binning_f != _binning_s) {
     snprintf(msgBuf, sizeof(msgBuf), "ERROR: binning_f is not equal to binning_s; Rayonix is not calibrated for this\n");
     fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
     if (_occSend != NULL) {
        _occSend->userMessage(msgBuf);
     }
  }
     
  // If the readoutMode is Unknown, send an occurrence
  if (readoutMode == 0) {
    snprintf(msgBuf, sizeof(msgBuf), "ERROR: readoutMode 0 (Unknown) is unsupported");
    fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
    if (_occSend != NULL) {
      _occSend->userMessage(msgBuf);
    }
  }

  _count = 0;

  // create rayonix control object (to be deleted in unconfigure())
  _rnxctrl = new rayonix_control(_verbose, RNX_IPADDR, RNX_CONTROL_PORT);

  if (_rnxctrl) {
    do {
       printf("Calling _rnxctrl->connect()...\n");
      _rnxctrl->connect();
      status = _rnxctrl->status();
      if (status == Pds::rayonix_control::Unconfigured) {
        if (_rnxctrl->config(binning_f, binning_s, exposure, rawMode, readoutMode, trigger,
                             testPattern, darkFlag, deviceBuf)) {
          printf("ERROR: _rnxctrl->config() failed\n");
          break;    /* ERROR */
        } else {
          status = _rnxctrl->status();
          _binning_f = binning_f;
          _binning_s = binning_s;
          _exposure  = exposure;
          _darkFlag  = darkFlag;
          _readoutMode = readoutMode;
          _trigger   = trigger;
          if (deviceBuf) {
            // copy to _deviceID
            strncpy(_deviceID, deviceBuf, Pds::rayonix_control::DeviceIDMax);
            printf("Rayonix Device ID: \"%s\"\n", _deviceID);
          }
        }
        printf("Calling _rnxctrl->enable()...\n");
        if (_rnxctrl->enable()) {
           printf("ERROR: _rnxctrl->enable() failed (status==%d)\n", _rnxctrl->status());
          break;    /* ERROR */
        }
        numErrs = 0;  /* Success */
      } else {
         printf("ERROR: _rnxctrl->connect() failed (status==%d) in %s\n", _rnxctrl->status(), __PRETTY_FUNCTION__);
      }
    } while (0);
  } else {
    printf("ERROR: _rnxctrl is NULL in %s\n", __PRETTY_FUNCTION__);
  }

  return (numErrs);
 }

unsigned Pds::RayonixServer::unconfigure(void)
{
  unsigned numErrs = 0;

  if (_verbose) {
    printf(" *** entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_rnxctrl) {
    _rnxctrl->disable();
    _rnxctrl->reset();
    _rnxctrl->disconnect();
    if (_rnxctrl->status() == Pds::rayonix_control::Unconnected) {
      delete _rnxctrl;
      _rnxctrl = NULL;
    } else {
      printf("ERROR: _rnxctrl->status() != Unconnected in %s\n", __PRETTY_FUNCTION__);
      ++ numErrs;
    }
  } else {
    printf("Warning: _rnxctrl is NULL in %s\n", __PRETTY_FUNCTION__);
  }

  return (numErrs);
}


unsigned Pds::RayonixServer::endrun(void)
{
  return(0);
}


int Pds::RayonixServer::fetch( char* payload, int flags )
{
  uint16_t frameNumber;
  int width  = Pds::Rayonix_MX170HS::n_pixels_fast/_binning_f;
  int height = Pds::Rayonix_MX170HS::n_pixels_slow/_binning_s;
  int offset = 0;
  int size_npixels = width*height;
  int binning_f = 0;
  int binning_s = 0;
  int verbose = RayonixServer::verbose();
  int rv;
  int damage = 0;
  char msgBuf[80];

  if (verbose) {
     printf("Entered %s\n", __PRETTY_FUNCTION__);
  }

  if (_outOfOrder) {
    return(-1);
  }

  // read data from UDP socket
  offset += sizeof(Xtc);
  new (payload+offset) Pds::Camera::FrameV1::FrameV1(width, height, Pds::Rayonix_MX170HS::depth_bits, 0);

  rv = _rnxdata->readFrame(frameNumber, (payload+offset), MAXFRAME, binning_f, binning_s, verbose);
  _count++;

  if (verbose) {
     printf("%s: received frame #%u/%u (0x%x)\n", __PRETTY_FUNCTION__, frameNumber, _count, rv);
  }
  
  // Check for out of order condition
  if ((_count & 0xffff) != frameNumber) {
     snprintf(msgBuf, sizeof(msgBuf), "Rayonix ERROR:  count 0x%x != frameNumber 0x%x in %s\n", 
              _count, frameNumber, __FUNCTION__);
     fprintf(stderr, "%s: %s", __PRETTY_FUNCTION__, msgBuf);
     // latch error
     _outOfOrder = 1;
     if (_occSend) { 
        // send occurrence
        _occSend->userMessage(msgBuf);
        _occSend->outOfOrder();
     }
     return(-1);
  }

  if ((binning_f != _binning_f) || (binning_s != _binning_s)) {
     printf ("ERROR:  Rayonix device binning is %dx%d, expected %dx%d\n", 
             binning_f, binning_s, _binning_f, _binning_s);
     damage++;
  }
  // readFrame return -1 on ERROR, otherwise the number of 16-bit pixels read.
  else if (rv == -1) {
     if (verbose) {
        printf("ERROR:  readFrame return value is -1\n");
     }
     damage++;
  }
  // size is # of bytes, while readFrame returns # of 16-bit pixels read.
  else if (rv != size_npixels) {
     if (verbose) {
        printf("ERROR:  Returned frame size (# of 16 bit pixels) 0x%x != expected frame size 0x%x\n", 
            rv, size_npixels);
     }
     damage++;
  }

  if (damage) {
     memcpy(payload, &_xtcDamaged, sizeof(Xtc));
  }
  else {
     // Calculate xtc extent (rv is number of pixels -> convert to bytes)
     _xtc.extent = rv*Pds::Rayonix_MX170HS::depth_bytes + sizeof(Xtc);
     
     // copy xtc header to payload
     memcpy(payload, &_xtc, sizeof(Xtc));
  }

  return (_xtc.extent);
}
                               

unsigned Pds::RayonixServer::count() const
{
  return(_count-1);
}

bool Pds::RayonixServer::verbose() const
{
  return(_verbose);
}

void Pds::RayonixServer::setOccSend(RayonixOccurrence* occSend)
{
  _occSend = occSend;
}

