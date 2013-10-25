// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __RAYONIX_DATA_HH
#define __RAYONIX_DATA_HH

#include "rayonix_common.hh"

#define UDP_RCVBUF_SIZE     (64*1024*1024)

namespace Pds
{
  class rayonix_data;
}

class Pds::rayonix_data {

public:
  rayonix_data(unsigned bufsize, bool verbose);
  ~rayonix_data();

  int fd() const                  { return (_notifyFd); }
  int drainFd(int fd) const;
  int reset(bool verbose) const;
  int readFrame(uint16_t& frameNumber, char *payload, int payloadMax, int &binning_f, int &binning_s, bool verbose) const;

private:
  //
  // private variables
  //
  int         _notifyFd;
  int         _dataFdEven;
  int         _dataFdOdd;
  char *      _discard;
  unsigned    _bufsize;
};
  
#endif
