// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#ifndef __RAYONIX_CONTROL_HH
#define __RAYONIX_CONTROL_HH

#include <unistd.h>

#include "rayonix_common.hh"

namespace Pds
{
  class rayonix_control;
}

class Pds::rayonix_control {

public:
  rayonix_control(bool verbose, const char *host, int port);
  ~rayonix_control();

  int connect();
  int config(int binning_f, int binning_s, int exposure, int rawMode,
             int readoutMode, int trigger, int testPattern, uint8_t darkFlag,
             char *deviceID);

  int calib();
  int enable(void);
  int disable(void);
  int reset(void);
  int disconnect(void);
  int status(void);

  // states
  enum {Unconnected=0, Unconfigured=1, Configured=2, Enabled=3};
  enum {HostnameMax=20};
  enum {DeviceIDMax=40};

private:
  bool        _verbose;
  int         _port;
  char        _host[HostnameMax+1];
  bool        _connected;
  int         _control_fd;
};
  
#endif
