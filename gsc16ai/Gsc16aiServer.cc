#include "Gsc16aiServer.hh"
#include "pds/xtc/XtcType.hh"

#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

using namespace Pds;

Pds::Gsc16aiServer::Gsc16aiServer( const Src& client )
   : _xtc( _gsc16aiDataType, client ),
     _adc(NULL),
     _occSend(NULL),
     _outOfOrder(0),
     _task(new Task(TaskObject("waitevent")))
{
  // create pipe
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    // setup to read from pipe
    fd(_pfd[0]);
    // call routine() in separate task
    _task->call(this);
  }
}

// consider calling _task->destroy() in destructor

int Pds::Gsc16aiServer::payloadComplete(void)
{
  const Command cmd = Payload;
  int rv = ::write(_pfd[1],&cmd,sizeof(cmd));
  if (rv == -1) {
    fprintf(stderr, "%s write error: %s\n", __FUNCTION__, strerror(errno));
  }
  return (rv);
}

void Pds::Gsc16aiServer::routine()
{
  int fdadc;
  gsc_wait_t  gWait = {0, 0, AI32SSC_WAIT_GSC_IN_BUF_THR_L2H, 0, 0, 5000, 0};
  timespec sleepTime = {0, 100000000u};  // 0.1 sec

  while (1) {
    // Wait for ADC to be opened
    if (_adc && _adc->get_isOpen()) {
      fdadc = _adc->get_fd();
      break;
    }
    nanosleep(&sleepTime, NULL);
  }

  while (1) {
    // Wait for data
    gWait.flags = 0;                              // must initially be zero
    gWait.gsc = AI32SSC_WAIT_GSC_IN_BUF_THR_L2H;
    gWait.timeout_ms = 2000;                      // 2 second timeout
    if (ai32ssc_dsl_ioctl(fdadc, AI32SSC_IOCTL_WAIT_EVENT, &gWait)) {
      fprintf(stderr, "Error: ioctl AI32SSC_IOCTL_WAIT_EVENT\n");
    } else if (gWait.flags == GSC_WAIT_FLAG_TIMEOUT) {
      // timed out: wait again
      continue;
    } else if (gWait.flags == GSC_WAIT_FLAG_DONE) {
      // detected event: send notification
      payloadComplete();
      // ...then wait again
      continue;
    } else {
      // printf("ioctl AI32SSC_IOCTL_WAIT_EVENT returned, flags = 0x%08x\n", gWait.flags);
    }
    // an error occured.  delay one second to avoid spinning too fast.
    sleep(1);
  }   // end of while(1)
}

unsigned Pds::Gsc16aiServer::configure(const Gsc16aiConfigType& config)
{
  unsigned numErrs = 0;

  _count      = 0;
  _firstChan  = config.firstChan();
  _lastChan   = config.lastChan();
  _timeTagEnable = config.timeTagEnable();
  _autocalibEnable = config.autocalibEnable();

  if (_adc->get_isOpen()) {
    numErrs = _adc->configure(config);
    if (numErrs) {
      fprintf(stderr, "Gsc16ai: Failed to configure.\n");
    }
    if (numErrs && _occSend) {
      // send occurrence
      _occSend->userMessage("Gsc16ai: Failed to configure.\n");
    }
  } else {
    numErrs ++;
    if (_occSend) {
      char msgBuf[80];
      snprintf(msgBuf, 80, "Gsc16ai: Failed to open %s.\n", _adc->get_devName() );
      _occSend->userMessage(msgBuf);
    }
  }

  return (numErrs);
}

unsigned Pds::Gsc16aiServer::unconfigure(void)
{
  _count = 0;
  return ((unsigned) _adc->unconfigure());
}

int Pds::Gsc16aiServer::fetch( char* payload, int flags )
{
  int extraChans = _lastChan - _firstChan;

  if (_outOfOrder) {
    // error condition is latched
    return (-1);
  }

  Gsc16aiDataType *frame = (Gsc16aiDataType *)(payload + sizeof(Xtc));

  // calculate extent assuming one channel configured
  _xtc.extent = sizeof(Gsc16aiDataType) + sizeof(Xtc);

  // adjust extent if more than one channel configured
  if (extraChans > 0) {
    // maintain longword alignment
    _xtc.extent += ((extraChans * sizeof(int16_t)) + 3) & ~0x3; 
  }
  
  memcpy(payload, &_xtc, sizeof(Xtc));  // copy after adjusting extent

  // read from pipe
  int length = ::read(_pfd[0], &_cmd, sizeof(_cmd));

  if (length != sizeof(_cmd)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", length, __FUNCTION__);
    return (-1);
  } else if (_cmd == Payload) {
    // read the data from ADC
    if (_adc && _adc->get_isOpen()) {
      int fifoDataSize = Gsc16ai::NumChannels * sizeof(int32_t);  // 64
      int fdadc = _adc->get_fd();

      int rv = read(fdadc, (void *)(frame->_channelValue), fifoDataSize);
      if (rv == -1) {
        perror ("read");
        return (-1);
      }

      if (rv != fifoDataSize) {
        fprintf(stderr, "Error: %s requested %d bytes, read() returned %d\n",
                __FUNCTION__, fifoDataSize, rv);
        return (-1);
      }

      // verify that first channel flag is set
      if ((frame->_channelValue[1] & 0x8000) != 0x8000) {
        if (!_outOfOrder) {
          // latch error
          _outOfOrder = 1;
          fprintf(stderr, "Error: first channel flag not set; data could be out-of-order\n");
          if (_occSend) {
            // send occurrence
            _occSend->outOfOrder();
          }
        }
        return (-1);
      }

      // if first channel is not 0, relocate it
      if (_firstChan) {
        frame->_channelValue[0] = frame->_channelValue[_firstChan * 2];
      }

      // if multiple channels are configured, relocate them
      if (extraChans > 0) {
        for (int ii = 1; ii <= extraChans; ii++) {
          frame->_channelValue[ii] = frame->_channelValue[(_firstChan + ii) * 2];
        }
      }

      // verify that FIFO is empty
      int bufLevel = _adc->get_bufLevel();
      if (bufLevel > 0) {
        if (!_outOfOrder) {
          // latch error
          _outOfOrder = 1;
          if (_occSend) {
            // send occurrence
            _occSend->outOfOrder();
          }
          fprintf(stderr, "Error: FIFO level nonzero (%d) after read; data could be out-of-order\n",
                  bufLevel);
        }
        return (-1);
      }

      // timestamp support
      if (!_timeTagEnable) {
        // when hw timestamp feature is not configured, use sw clock for timestamp
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        frame->_timestamp[0] = (uint16_t) (ts.tv_sec % 3600);   // sec modulo 1hr
        frame->_timestamp[1] = (uint16_t) (ts.tv_nsec >> 16);   // nsec upper half
        frame->_timestamp[2] = (uint16_t) ts.tv_nsec;           // nsec lower half
      }

      // success
      ++_count;
      return (_xtc.extent);

    } else {
      fprintf(stderr, "Error: ADC is not open in %s\n", __FUNCTION__);
      return (-1);
    }

  } else {
    printf("Unknown command (0x%x) in %s\n", _cmd, __FUNCTION__);
    return (-1);
  }
}

unsigned Gsc16aiServer::count() const
{
  return (_count-1);
}

void Gsc16aiServer::setAdc(gsc16ai_dev* adc)
{
  _adc = adc;
}

void Gsc16aiServer::setOccSend(Gsc16aiOccurrence* occSend)
{
  _occSend = occSend;
}

bool Gsc16aiServer::get_autocalibEnable()
{
  return (_autocalibEnable);
}

//
// calibrate - initiate an auto-calibration cycle
//
int Gsc16aiServer::calibrate()
{
  int rv = -1;
  if (_adc == NULL) {
    fprintf(stderr, "Error: ADC is not open in %s\n", __FUNCTION__);
  } else {
    rv = _adc->calibrate();
  }
  return (rv);
}
