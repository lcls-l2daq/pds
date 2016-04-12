#include "pds/lecroy/Server.hh"
#include "pds/lecroy/ConfigServer.hh"
#include "pds/xtc/InDatagram.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/Generic1DConfigType.hh"
#include "pds/config/Generic1DDataType.hh"
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

using namespace Pds::LeCroy;

const static int CON_POLL  = 100000;

Server::Server(const char* pvbase,
               const Pds::DetInfo& info,
               const unsigned max_length) :
  _xtc        (_generic1DDataType,info),
  _max_length (max_length),
  _count      (0),
  _readout    (true),
  _ready      (false),
  _raw        (NCHANNELS),
  _config_pvs (NCHANNELS),
  _offset_pvs (NCHANNELS),
  _period_pvs (NCHANNELS),
  _enabled    (false),
  _chan_mask  (0),
  _config_size(0),
  _data_size  (0),
  _ConfigBuff (0),
  _DataBuff   (0),
  _sem        (Semaphore::FULL)
{
  int err = ::pipe(_pfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  } else {
    fd(_pfd[0]);
  }

  //
  //  Create ConfigServers for fetching configuration data
  //
  char pvname[64];
  sprintf(pvname,"%s:INR_CALC",pvbase);
  _status = new ConfigServer(pvname,this,true);

  for(unsigned i=0; i<NCHANNELS; i++) {
    sprintf(pvname,"%s:NUM_SAMPLES_C%d",pvbase, i+1);
#ifdef DBUG
    printf("Creating EpicsCA(%s)\n",pvname);
#endif
    _config_pvs[i] = new ConfigServer(pvname,this);

    sprintf(pvname,"%s:FIRST_PIXEL_OFFSET_C%d",pvbase, i+1);
#ifdef DBUG
    printf("Creating EpicsCA(%s)\n",pvname);
#endif
    _offset_pvs[i] = new ConfigServer(pvname,this);

    sprintf(pvname,"%s:HORIZ_INTERVAL_C%d",pvbase, i+1);
#ifdef DBUG
    printf("Creating EpicsCA(%s)\n",pvname);
#endif
    _period_pvs[i] = new ConfigServer(pvname,this);

    sprintf(pvname,"%s:TRACE_C%d",pvbase, i+1);
    _raw[i] = new Pds_Epics::PVSubWaveform(pvname,this);

    // initialize sample type array
    _SampleType[i] = Generic1DConfigType::FLOAT64;

    // retrieve the current trace length
    while(!_config_pvs[i]->connected() || !_raw[i]->connected()) {
      usleep(CON_POLL);
    }

    // set the number of elements for the CA monitor to max EB can handle
    _raw[i]->set_nelements(_max_length);
    _raw[i]->start_monitor();
  }

  // Wait for monitors to be established
  ca_pend_io(0);

  // allocate a buffer for storing config objects
  _config_size = Generic1DConfigType(NCHANNELS, 0, 0, 0, 0)._sizeof();
  _ConfigBuff = new char[_config_size];
}

Server::~Server()
{
  delete _status;
  for(unsigned i=0; i<_raw.size(); i++)
    delete _raw[i];
  for(unsigned i=0; i<_config_pvs.size(); i++)
    delete _config_pvs[i];
  for(unsigned i=0; i<_offset_pvs.size(); i++)
    delete _offset_pvs[i];
  for(unsigned i=0; i<_period_pvs.size(); i++)
    delete _period_pvs[i];
  if(_ConfigBuff)
    delete[] _ConfigBuff;
  if(_DataBuff)
    delete[] _DataBuff;
}

int Server::fetch( char* payload, int flags )
{
  void* p;
  int len = ::read(_pfd[0], &p, sizeof(p));
  if (len != sizeof(p)) {
    fprintf(stderr, "Error: read() returned %d in %s\n", len, __FUNCTION__);
    return -1;
  }

  _count++;
#ifdef DBUG
  printf("Scope readout count: %d\n", _count);
#endif
  return fill(payload,p);
}

void Server::post(const void* p)
{
  ::write(_pfd[1], &p, sizeof(p));
}

unsigned Server::count() const
{
  return _count - 1;
}

//
//  Called by "post" function to write event data into the XTC
//
int Server::fill(char* copyTo, const void* data)
{
  //
  //  Set the xtc header
  //
  char* p = copyTo;
  Xtc& xtc = *new (p) Xtc(_xtc);
  p += sizeof(Xtc);

  //
  //  Parse the raw data and fill the copyTo payload
  //

  uint32_t payload_size = _data_size + sizeof(Generic1DDataType);

  new(xtc.alloc(payload_size)) Generic1DDataType(_data_size, (const uint8_t*) data);

  _xtc.extent = xtc.extent;

#ifdef DBUG
  printf("Filled xtc data with extent: %d\n", xtc.extent); 
#endif

  return xtc.extent;
}

void Server::signal(bool isTrig)
{
  if (_enabled) {
    _sem.take();
    if (isTrig) {
      fetch_readout();
      if (!_readout)
        printf("Scope readout has timed out!\n");
#ifdef DBUG
      else
        printf("Scope readout has succeeded!\n");
#endif
    } else {
      if (_ready)
        printf("Scope configuration has changed during a run!\n");
      _ready = false;
    }
    _sem.give();
  }
}

void Server::fetch_readout()
{
  double status_val = 0.;
  _status->fetch((char*)&status_val);
  if (status_val < 0.5) _readout=false;
  else _readout=true;
}

//
//  Callback when new event data is available
//
void Server::updated()
{
  if (_enabled) {
    _sem.take();
    // report a trace being received
    _chan_mask++;
#ifdef DBUG
    printf("Received camonitor callback on trace data: %d of %d\n", _chan_mask, NCHANNELS);
#endif
    if(_chan_mask == NCHANNELS) {
      double* dataptr = reinterpret_cast<double*>(_DataBuff);
      for(unsigned i=0; i<NCHANNELS; i++) {
        if (_readout)
          memcpy(dataptr, _raw[i]->data(), _Length[i]*sizeof(double));
        else 
          memset(dataptr, 0, _Length[i]*sizeof(double));
        dataptr += _Length[i];
      }
      post(_DataBuff);
      _chan_mask = 0;
    }
    _sem.give();
  }
}

Pds::Transition* Server::fire(Pds::Transition* tr)
{
  switch (tr->id()) {
  case Pds::TransitionId::Enable:
    _sem.take();
    fetch_readout();
    _chan_mask=0;
    _enabled=true;
    _sem.give();
    break;
  case Pds::TransitionId::Disable:
    _sem.take();
    _enabled=false;
    _sem.give();
    break;
  case Pds::TransitionId::L1Accept:
  default:
    break;
  }
  return tr;
}

Pds::InDatagram* Server::fire(Pds::InDatagram* dg)
{
  if (dg->seq.service()==Pds::TransitionId::Configure) {
    //
    //  Fetch the configuration data for inserting into the XTC
    //
    uint32_t size_calc = 0;
    for(unsigned i=0; i<NCHANNELS; i++) {
      printf("Fetching configuration for trace %d\n", i+1);
      _config_pvs[i]->fetch((char*)&_Length[i]);
      _offset_pvs[i]->fetch((char*)&_RawOffset[i]);
      _period_pvs[i]->fetch((char*)&_Period[i]);

      // Calucate integer offset to put in the config
      _Offset[i] = (int32_t) (_RawOffset[i]/_Period[i]);

      size_calc += _Length[i]*sizeof(double);

      printf("_Length[%d]=%d\n",i,_Length[i]);
      printf("_Offset[%d]=%d\n",i,_Offset[i]);
      printf("_Period[%d]=%g\n",i,_Period[i]);
     
      if(_Length[i] > _max_length) {
        printf("Invalid configuration: trace %d has %d elements and current max is %d\n",
               i+1,
               _Length[i],
               _max_length);
        dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
      }
    }

    // reset readout count to zero
    _count = 0;
    // set config ready flag
    _sem.take();
    _ready = true;
    _sem.give();

    Pds::Xtc* xtc = new ((char*)dg->xtc.next())
      Pds::Xtc( _generic1DConfigType, _xtc.src );
    new (xtc->alloc(_config_size)) Generic1DConfigType(NCHANNELS, _Length, _SampleType, _Offset, _Period);
    dg->xtc.alloc(xtc->extent);
    new (_ConfigBuff) Generic1DConfigType(NCHANNELS, _Length, _SampleType, _Offset, _Period);
    if(_DataBuff) {
      if(size_calc > _data_size) {
        delete[] _DataBuff;
        _DataBuff = new char[size_calc];
      }
    } else {
      _DataBuff = new char[size_calc];
    }
    _data_size = size_calc;
  } else if (dg->seq.service()==Pds::TransitionId::L1Accept) {
    if(!_readout) {
      printf("Scope readout count %d marked as damaged due to trigger timeout: fid %08X\n",
             _count,
             dg->datagram().seq.stamp().fiducials());
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    } else if(!_ready) {
      printf("Scope readout count %d marked as damaged due to configuration change: fid %08X\n",
             _count,
             dg->datagram().seq.stamp().fiducials());
      dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
    }
  }
  return dg;
}
