#ifndef __RAYONIXSERVER_HH
#define __RAYONIXSERVER_HH

#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <vector>

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/RayonixConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Semaphore.hh"
#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/xtc/Xtc.hh"

#include "rayonix_control.hh"
#include "rayonix_data.hh"
#include "RayonixOccurrence.hh"

namespace Pds
{
  class RayonixServer;
}

class Pds::RayonixServer 
  : public EbServer,
    public EbCountSrv
{
public:
  RayonixServer(const Src& client, 
                bool verbose);
  virtual ~RayonixServer() {}

  // Eb interface
  void       dump     ( int detail ) const {}
  bool       isValued ( void )       const {return true;}
  const Src& client   ( void )       const {return _xtc.src; }

  // EbSegment interface
  const Xtc& xtc    ( void ) const { return _xtc; }
  unsigned   offset ( void ) const { return sizeof(Xtc); }
  unsigned   length ()       const { return _xtc.extent; }

  // Eb-key interface
  EbServerDeclare;

  // Server interface
  int pend  ( int flag = 0 )              { return -1; }
  int fetch ( ZcpFragment&,  int flags )  { return 0; }
  int fetch ( char* payload, int flags );

  // Misc
  unsigned count() const;
  bool     verbose  () const;
  void     reset    () { _outOfOrder=0; }

  unsigned configure  ( RayonixConfigType& config );
  unsigned unconfigure( void );
  unsigned endrun     ( void );


  void setOccSend(RayonixOccurrence* occSend);

  // private variables
  Xtc      _xtc;
  Xtc      _xtcDamaged;
  unsigned _count;
  RayonixOccurrence *_occSend;
  int       _outOfOrder;
  bool      _verbose;
  int       _binning_f;
  int       _binning_s;
  uint32_t  _exposure;
  uint16_t  _darkFlag;
  uint16_t  _readoutMode;
  uint32_t  _trigger;
  char      _deviceID[Pds::rayonix_control::DeviceIDMax + 1];
  rayonix_control * _rnxctrl;
  rayonix_data *    _rnxdata;
};

#endif

