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
     _buffer(new vector<BufferElement>(BufferDepth)),
     _shutdownFlag(false),
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

int Pds::Gsc16aiServer::payloadComplete(vector<BufferElement>::iterator buf_iter)
{
  int rv;
  command_t sendCommand;

  sendCommand.cmd = Payload;
  sendCommand.buf_iter = buf_iter;

  rv = ::write(_pfd[1],&sendCommand,sizeof(sendCommand));
  if (rv == -1) {
    fprintf(stderr, "%s write error: %s\n", __FUNCTION__, strerror(errno));
  }
  return (rv);
}

void Pds::Gsc16aiServer::routine()
{
  timespec sleepTime = {0, 100000000u};  // 0.1 sec
  int waitResult;
  vector<BufferElement>::iterator buf_iter;

  while (1) {
    // Wait for ADC to be opened
    if (_adc && _adc->get_isOpen()) {
      break;
    }
    nanosleep(&sleepTime, NULL);
  }

  while (1) {
    for (buf_iter = _buffer->begin(); buf_iter != _buffer->end(); ) {
      // Wait for data
      waitResult = _adc->waitEventInBufThrL2H(500);     // half second timeout

      if (_shutdownFlag) {
        // exit the task
        goto routine_shutdown;
      }

      if (waitResult == gsc16ai_dev::waitEventReady) {
        // detected event...

        if (buf_iter->_full) {
          // buffer is full: error
          fprintf(stderr, "Error: buffer overrun in %s\n", __PRETTY_FUNCTION__);
          if (_occSend) {
            // send occurrence
            _occSend->outOfOrder();
            _occSend->userMessage("Gsc16ai: Buffer overrun\n");
          }
          // exit the task
          goto routine_shutdown;

        } else {
          // buffer is empty: mark it as full
          buf_iter->_full = true;
        }

        // ...read the data from ADC
        int fifoDataSize = sizeof(buf_iter->_channelValue);
        int fdadc = _adc->get_fd();

        int rv = read(fdadc, (void *)(buf_iter->_channelValue), fifoDataSize);
        if (rv == -1) {
          perror ("read");
          // exit the task
          goto routine_shutdown;
        }

        if (rv != fifoDataSize) {
          fprintf(stderr, "Error: %s requested %d bytes, read() returned %d\n",
                  __FUNCTION__, fifoDataSize, rv);
          // exit the task
          goto routine_shutdown;
        }

        // verify that first channel flag is set
        if ((buf_iter->_channelValue[1] & 0x8000) != 0x8000) {
          if (!_outOfOrder) {
            // latch error
            _outOfOrder = 1;
            fprintf(stderr, "Error: first channel flag not set; data could be out-of-order\n");
            if (_occSend) {
              // send occurrence
              _occSend->outOfOrder();
              _occSend->userMessage("Gsc16ai: First channel flag not set\n");
            }
          }
          // exit the task
          goto routine_shutdown;
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
              _occSend->userMessage("Gsc16ai: FIFO not empty after read\n");
            }
            fprintf(stderr, "Error: FIFO level nonzero (%d) after read; data could be out-of-order\n",
                    bufLevel);
          }
          // exit the task
          goto routine_shutdown;
        }

        // timestamp support
        if (!_timeTagEnable) {
          // when hw timestamp feature is not configured, use sw clock for timestamp
          timespec ts;
          clock_gettime(CLOCK_REALTIME, &ts);
          buf_iter->_timestamp[0] = (uint16_t) (ts.tv_sec % 3600);  // sec modulo 1hr
          buf_iter->_timestamp[1] = (uint16_t) (ts.tv_nsec >> 16);  // nsec upper half
          buf_iter->_timestamp[2] = (uint16_t) ts.tv_nsec;          // nsec lower half
        }

        // ...send notification
        payloadComplete(buf_iter);

        // ...advance to next buffer, then wait again
        buf_iter++;
        continue;

      } else if (waitResult == gsc16ai_dev::waitEventTimeout) {
        // ...wait again
        continue;

      } else {
        fprintf(stderr, "Error: waitEventInBufThrL2H() returned %d\n", waitResult);
      }
      // an error occured.  delay to avoid spinning too fast.
      nanosleep(&sleepTime, NULL);
    }   // end of for(...)
  }   // end of while(1)

routine_shutdown:
  printf("\n ** %s shutdown **\n", __PRETTY_FUNCTION__);
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
  command_t receiveCommand;
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
  int length = ::read(_pfd[0], &receiveCommand, sizeof(receiveCommand));

  if (length != sizeof(receiveCommand)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", length, __FUNCTION__);
    return (-1);
  } else if (receiveCommand.cmd == Payload) {
    vector<BufferElement>::iterator buf_iter = receiveCommand.buf_iter;

    // if first channel is not 0, relocate it
    if (_firstChan) {
      buf_iter->_channelValue[0] = buf_iter->_channelValue[_firstChan * 2];
    }

    // if multiple channels are configured, relocate them
    if (extraChans > 0) {
      for (int ii = 1; ii <= extraChans; ii++) {
        buf_iter->_channelValue[ii] = buf_iter->_channelValue[(_firstChan + ii) * 2];
      }
    }

    // copy the received data
    for (int ii = 0; ii < extraChans + 1; ii++) {
      frame->_channelValue[ii] = buf_iter->_channelValue[ii];
    }
    frame->_timestamp[0] = buf_iter->_timestamp[0];
    frame->_timestamp[1] = buf_iter->_timestamp[1];
    frame->_timestamp[2] = buf_iter->_timestamp[2];

    // mark buffer as empty
    buf_iter->_full = false;

    // success
    ++_count;
    return (_xtc.extent);
  } else {
    printf("Unknown command (0x%x) in %s\n", receiveCommand.cmd, __FUNCTION__);
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
