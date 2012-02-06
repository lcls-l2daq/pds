/*
 * PhasicsServer.cc
 *
 *  Created on: Nov 15, 2010
 *      Author: jackp
 */

#include "pds/phasics/PhasicsServer.hh"
#include "pds/phasics/PhasicsConfigurator.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/xtc/ZcpDatagram.hh"
#include "pds/config/PhasicsConfigType.hh"
#include "pds/camera/FrameType.hh"
#include "pds/service/Task.hh"
#include "pds/service/TaskObject.hh"
#include "pds/phasics/PhasicsConfigurator.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/camera/FrameV1.hh"
#include <dc1394/dc1394.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

void sigHandler( int signal ) {
  dc1394camera_t *camera = Pds::PhasicsServer::instance()->camera();
  psignal( signal, "Signal received by PhasicsServer");
  if (camera) {
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_camera_free(camera);
  } else printf("No camera found!\n");
  exit(signal);
}

using namespace Pds;
//using namespace Pds::Phasics;

PhasicsServer* PhasicsServer::_instance = 0;

class Task;
class TaskObject;
class DetInfo;

long long int timeDiff(timespec* end, timespec* start) {
  long long int diff;
  diff =  (end->tv_sec - start->tv_sec);
  if (diff) diff *= 1000000000LL;
  diff += end->tv_nsec;
  diff -= start->tv_nsec;
  return diff;
}

enum {PreadPipe=0, PwritePipe=1};

unsigned PhasicsReceiver::count = 0;
bool     PhasicsReceiver::first = true;

void PhasicsReceiver::printError(char* m) {
  if ((_err>0)||(_err<=-DC1394_ERROR_NUM))
    _err=DC1394_INVALID_ERROR_CODE;
  if (_err!=DC1394_SUCCESS) {
    printf("%s: in %s (%s, line %d): %s\n",
        dc1394_error_get_string(_err),
        __FUNCTION__, __FILE__, __LINE__, m);
  }
}

void PhasicsReceiver::waitForNotFirst() {
  while (first) {
    usleep(1000);
  }
}

void PhasicsReceiver::routine(void) {
  enum {SelectSleepTimeUSec=2000};
  unsigned mod = 1000;
  _err = DC1394_SUCCESS;
  runFlag = true;
  int             fd = 0;
  int             f;
  struct timeval  timeout;
  timeout.tv_sec  = 0;
  timeout.tv_usec = SelectSleepTimeUSec;

  printf("PhasicsReceiver::routine starting run loop\n");

  while (runFlag) {
    f = dc1394_capture_get_fileno(_camera);
    if (f!=fd) {
      fd = f;
      printf("PhasicsReceiver::routine got camera fileno %d\n", fd);
    }
      frame = 0;
      _err = dc1394_capture_dequeue(_camera, DC1394_CAPTURE_POLICY_POLL, &frame);
      printError("PhasicsReceiver::routine dequeue failed");
      if (_err != DC1394_SUCCESS) runFlag = false;
      if (frame) {
        if (first==false) {
          if ((count % mod) == 0) {
            printf("PhasicsReceiver::routine dequeued frame(%u) timestamp(%llu)\n", count, (long long unsigned) frame->timestamp);
          }
          count++;
          write(out[PwritePipe], &frame, sizeof(frame));
          read(in[PreadPipe], &frame, sizeof(frame));
        } else {
          printf("PhasicsReceiver::routine threw away frame(%u) timestamp(%llu)\n", count, (long long unsigned) frame->timestamp);
          first = false;
        }
        _err = dc1394_capture_enqueue(_camera, frame);
        printError("PhasicsReceiver::routine enqueue failed");
      }
      usleep(2000);
  }
  printf("PhasicsReceiver::routine exiting\n");
}

PhasicsServer::PhasicsServer( const Pds::Src& client )
   : _xtc( _frameType, client ),
     _cnfgrtr(0),
     _payloadSize(Pds::Phasics::ConfigV1::Width*Pds::Phasics::ConfigV1::Height*((Pds::Phasics::ConfigV1::Depth+7)>>3) +
         sizeof(Pds::Xtc) + sizeof(Pds::Camera::FrameV1)),
     _imageSize(Pds::Phasics::ConfigV1::Width*Pds::Phasics::ConfigV1::Height*(Pds::Phasics::ConfigV1::Depth+7)>>3),
     _configureResult(0),
     _debug(0),
     _task(0),
     _receiver(0),
     _unconfiguredErrors(0),
     _configured(false),
     _firstFetch(true),
     _enabled(false) {
  _histo = (unsigned*)calloc(sizeOfHisto, sizeof(unsigned));
  instance(this);
  if (pipe(_s2rFd) == -1) { perror("Server to Receiver pipe"); exit(EXIT_FAILURE); }
  if (pipe(_r2sFd) == -1) { perror("Receiver to Server pipe"); exit(EXIT_FAILURE); }
  fd(_r2sFd[PreadPipe]);
  printf("PhasicsServer::PhasicsServer() payload(%u) fd(%d)\n", _payloadSize, _r2sFd[PreadPipe]);
  signal( SIGINT,  sigHandler );
  signal( SIGQUIT, sigHandler );
  signal( SIGILL,  sigHandler );
  signal( SIGABRT, sigHandler );
  signal( SIGFPE,  sigHandler );
  signal( SIGKILL, sigHandler );
  signal( SIGSEGV, sigHandler );
  signal( SIGPIPE, sigHandler );
  signal( SIGTERM, sigHandler );
  signal( SIGBUS,  sigHandler );
}

void PhasicsServer::printError(char* m) {
  if ((_err>0)||(_err<=-DC1394_ERROR_NUM))
    _err=DC1394_INVALID_ERROR_CODE;
  if (_err!=DC1394_SUCCESS) {
    printf("%s: in %s (%s, line %d): %s\n",
        dc1394_error_get_string(_err),
        __FUNCTION__, __FILE__, __LINE__, m);
  }
}

unsigned PhasicsServer::configure(PhasicsConfigType* config) {
  if (_cnfgrtr == 0) {
    _cnfgrtr = new Pds::Phasics::PhasicsConfigurator::PhasicsConfigurator();
  }

  if ((_configureResult = _cnfgrtr->configure(config))) {
    printf("PhasicsServer::configure failed 0x%x\n", _configureResult);
  } else {
    _xtc.extent = _payloadSize;
    printf("PhasicsServer::configure _payloadSize(%u) _xtc.extent(%u)\n",
        _payloadSize, _xtc.extent);
  }
  _firstFetch = true;
  _count = 0;
  PhasicsReceiver::resetCount();
  _configured = _configureResult == 0;
  return _configureResult;
}

void PhasicsServer::allocated() {
}

void Pds::PhasicsServer::enable() {
  dc1394camera_list_t *      list;
  if (_debug & 0x20) printf("PhasicsServer::enable\n");
  _err=dc1394_camera_enumerate (_cnfgrtr->getD(), &list);
  printError("Failed to enumerate cameras");
  if (list->num == 0) {
    printf("PhasicsServer::enable no cameras found");
  }
  // assume we are using the first camera we found.
  // if there are more than one, we'll have to qualify the one we want
  _camera = dc1394_camera_new (_cnfgrtr->getD(), list->ids[0].guid);
  if (!_camera) {
    printf("Failed to initialize camera with guid %llx\n", (long long unsigned)list->ids[0].guid);
  }
  dc1394_camera_free_list (list);

  _err=dc1394_capture_setup(_camera,10, DC1394_CAPTURE_FLAGS_DEFAULT);
  printError( "Could not setup camera for capture");
  int f = dc1394_capture_get_fileno(_camera);
  printf("PhasicsServer::enable got camera fileno %d\n", f);
  if (!_task) _task = new Pds::Task(Pds::TaskObject("PhasicsReceiver"));
  if (_receiver==0) {
    printf("PhasicsServer::enable starting receiver\n");
    _receiver = new PhasicsReceiver(_camera, _s2rFd, _r2sFd);
  }
  _receiver->camera(_camera);
  PhasicsReceiver::resetFirst();
  _task->call(_receiver);
  _err=dc1394_video_set_transmission(_camera, DC1394_ON);
  printError( "Could not start camera iso transmission");


  _err=dc1394_external_trigger_set_power(_camera, DC1394_ON);
  printError( "Could not set trigger to DC1394_ON");
  _receiver->waitForNotFirst();
  _firstFetch = true;
}

void Pds::PhasicsServer::disable() {
  if (_debug & 0x20) printf("PhasicsServer::disable\n");
  _receiver->die();

  _err=dc1394_external_trigger_set_power(_camera, DC1394_OFF);
  printError( "Could not set trigger to DC1394_ON");

  _err=dc1394_video_set_transmission(_camera, DC1394_OFF);
  printError( "Could not stop camera iso transmission");
  _err=dc1394_capture_stop(_camera);
  printError( "Could not stop capture");
  dc1394_camera_free(_camera);
}

unsigned Pds::PhasicsServer::unconfigure(void) {
  return 0;
}

int Pds::PhasicsServer::fetch( char* payload, int flags ) {
   int ret = 0;
   unsigned        offset = 0;
   enum {Ignore=-1};

   if (_configured == false)  {
     if (++_unconfiguredErrors<20) printf("PhasicsServer::fetch() called before configuration, configuration result 0x%x\n", _configureResult);
     return Ignore;
   }
   if (_debug & 1) printf("PhasicsServer::fetch called ");
   _xtc.damage = 0;

   memcpy( payload, &_xtc, sizeof(Xtc) );
   offset += sizeof(Xtc);
   if (_firstFetch) {
     _firstFetch = false;
     clock_gettime(CLOCK_REALTIME, &_lastTime);
   } else {
     clock_gettime(CLOCK_REALTIME, &_thisTime);
     long long unsigned diff = timeDiff(&_thisTime, &_lastTime);
     diff += 500000;
     diff /= 1000000;
     if (diff > sizeOfHisto-1) diff = sizeOfHisto-1;
     _histo[diff] += 1;
     memcpy(&_lastTime, &_thisTime, sizeof(timespec));
   }

   if ((ret = read(_r2sFd[PreadPipe], &_frame, sizeof(_frame))) < 0) {
     perror ("PhasicsServer::fetch read error");
     ret =  Ignore;
   } else {
     new (payload+offset) Pds::Camera::FrameV1::FrameV1(
         Pds::Phasics::ConfigV1::Width,
         Pds::Phasics::ConfigV1::Height,
         Pds::Phasics::ConfigV1::Depth,
         0);
     offset += sizeof(Pds::Camera::FrameV1);
     swab(_frame->image, payload + offset, _imageSize);
     if ((ret = write(_s2rFd[PwritePipe], &_frame, sizeof(_frame))) < 0) {
       perror ("PhasicsServer::fetch write error");
     }
     ret = _payloadSize;
   }

   if (_debug & 5) printf(" returned %d frame %d\n", ret, _count);
   _count++;

   return ret;
}

bool PhasicsServer::more() const {
  bool ret = false;
  if (_debug & 2) printf("PhasicsServer::more(%s)\n", "false");
  return ret;
}

unsigned PhasicsServer::offset() const {
  unsigned ret =  0;
  if (_debug & 2) printf("PhasicsServer::offset(%u)\n", ret);
  return (ret);
}

unsigned PhasicsServer::count() const {
  if (_debug & 2) printf( "PhasicsServer::count(%u)\n", _count);
  return _count-1;
}

void PhasicsServer::printHisto(bool c) {
  printf("PhasicsServer event fetch periods\n");
  for (unsigned i=0; i<sizeOfHisto; i++) {
    if (_histo[i]) {
      printf("\t%3u ms   %8u\n", i, _histo[i]);
      if (c) _histo[i] = 0;
    }
  }
}

#undef PRINT_ERROR
