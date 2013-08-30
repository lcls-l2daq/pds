#ifndef __GSC16AI_DEV_HH
#define __GSC16AI_DEV_HH

#include <unistd.h>

#include "pdsdata/psddl/gsc16ai.ddl.h"
#include "16ai32ssc_dsl.hh"

namespace Pds
{
  class gsc16ai_dev;

  namespace Gsc16ai {
    enum {NumChannels = 16};
    enum {BitsPerChannel = 16};
  }
}

class Pds::gsc16ai_dev {
  public:
    gsc16ai_dev(const char* devName);
    ~gsc16ai_dev() {}
    int open( void );
    int close( void );
    int read(uint32_t *buf, size_t samples);
    int get_fd() { return _fd; }
    int get_isOpen() { return _isOpen; }
    int get_nChan() { return _nChan; }
    const char *get_devName() { return _devName; }
    int configure(const Pds::Gsc16ai::ConfigV1& config);
    int unconfigure(void);
    int get_bufLevel();
    int calibrate();
    enum {waitEventReady = 1, waitEventTimeout = 2, waitEventError = 3};
    int waitEventInBufThrL2H(int timeout_ms);

 private:
   const char * _devName;
   int          _fd;
   bool         _isOpen;
   int          _nChan;
};
  
#endif
