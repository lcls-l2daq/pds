//! FccdCamera.hh
//! FccdCamera camera control class. This class inherits of PicPortCL, which
//! means it assumes the Camera is connected to a Leutron frame grabber.
//!
//! Copyright 2010, SLAC
//! Author: caf@slac.stanford.edu
//!

#ifndef Pds_FccdCamera_hh
#define Pds_FccdCamera_hh

#include "pds/camera/CameraBase.hh"
#include "pds/config/FccdConfigType.hh"

#define FCCD_NAME             "LBNL_FCCD"

namespace Pds {
  class FccdCamera : public CameraBase {
  public:
    FccdCamera();
    virtual ~FccdCamera();

    void                  set_config_data(const void*);
    const FccdConfigType& Config() const;

  public:
    int configure(CameraDriver&,
		  UserMessage*);
    bool validate(Pds::FrameServerMsg&);
  private:
    //  Serial command interface
    virtual int           baudRate() const { return 115200; }
    virtual char          eotWrite() const { return 0x06; } // unused
    virtual char          eotRead () const { return 0x03; }
    virtual char          sof     () const { return '@'; }  // unused
    virtual char          eof     () const { return '\r'; } // unused
    virtual unsigned long timeout_ms() const { return 10000; }
  private:
    virtual const char* camera_name  () const;
    virtual int         camera_depth () const;
    virtual int         camera_width () const;
    virtual int         camera_height() const;
    virtual int         camera_taps  () const;
  private:
    int SendFccdCommand(CameraDriver&, 
			const char *cmd);
    int ShiftDataModulePhase(CameraDriver&,
			     int moduleNum);
  private:
    unsigned long LastCount;
    unsigned long CurrentCount;
    const FccdConfigType* _inputConfig;
    char* _outputBuffer;
  };

}

#endif
