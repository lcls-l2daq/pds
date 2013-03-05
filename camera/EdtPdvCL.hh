//! EdtPdvCL.hh
//! Camera class child that uses the EDT Pdv API to
//! control a Camera connected to a Pdv frame grabber using
//! the Camera Link protocol.
//!
//! GPL license
//!

#ifndef PDS_CAMERA_EDTPDVCL
#define PDS_CAMERA_EDTPDVCL

#include "pds/camera/CameraDriver.hh"
#include "pds/mon/THist.hh"

#include <edtinc.h>  // conflict with pds/service/Semaphore.hh

namespace Pds {

  class FrameServerMsg;
  class GenericPool;
  class MonEntryTH1F;

  class EdtReader;
  class MyQueue;

  class EdtPdvCL: public CameraDriver {
  public:
    EdtPdvCL(CameraBase&, int unit, int channel);
    virtual ~EdtPdvCL();
  public:
    PdvDev* dev() { return _dev; }
  public:  // Camera interface
    int initialize       (UserMessage*);
    int start_acquisition(FrameServer&,  // post frames here
                          Appliance  &,  // post occurrences here
			  UserMessage*);
    int stop_acquisition();
    void handle         (u_char*);
    void handle_error   (const char*);
  public:
    int SendCommand(char* szCommand, 
		    char* pszResponse, 
		    int   iResponseBufferSize);
    int SendBinary (char* szCommand, 
		    int   iCommandLen,
		    char* pszResponse, 
		    int   iResponseBufferSize);
    void dump();
  protected: 
    void _setup(int unit, int channel);
    int                  _unit;
    int                  _channel;
    int                  _tmo_ms;
    PdvDev*              _dev;
    EdtReader*           _acq;
  private:
    FrameServer*    _fsrv;
    Appliance*      _app;
    GenericPool*    _occPool;
    bool                 _outOfOrder;
    bool                 _latched;
    unsigned             _nposts;
    timespec             _tsignal;
    THist           _hsignal;
  private:
    MonEntryTH1F* _depth;
    MonEntryTH1F* _interval;
    MonEntryTH1F* _interval_w;
    unsigned           _sample_sec;
    unsigned           _sample_nsec;
  };
  
};

#endif
