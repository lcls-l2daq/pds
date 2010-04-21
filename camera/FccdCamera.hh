//! FccdCamera.hh
//! FccdCamera camera control class. This class inherits of PicPortCL, which
//! means it assumes the Camera is connected to a Leutron frame grabber.
//!
//! Copyright 2010, SLAC
//! Author: caf@slac.stanford.edu
//!

#ifndef Pds_FccdCamera_hh
#define Pds_FccdCamera_hh

#include "pds/camera/PicPortCL.hh"
#include "pds/config/FccdConfigType.hh"

#define FCCD_NAME             "LBNL_FCCD"

namespace PdsLeutron {

  class FccdCamera : public PicPortCL {
  public:
    FccdCamera(char *id = NULL, unsigned grabberId=0, const char *grabberName = "Stereo");
    virtual ~FccdCamera();

    void                    Config(const FccdConfigType&);
    const FccdConfigType& Config() const;

  private:
    //  Serial command interface
    virtual int           baudRate() const { return 115200; }
    virtual unsigned long parity  () const { return LvComm_ParityNone; }
    virtual unsigned long byteSize() const { return LvComm_Data8; }
    virtual unsigned long stopSize() const { return LvComm_Stop1; }
    virtual char          eotWrite() const { return 0x06; } // unused
    virtual char          eotRead () const { return 0x03; }
    virtual char          sof     () const { return '@'; }  // unused
    virtual char          eof     () const { return '\r'; } // unused
    virtual unsigned long timeout_ms() const { return 10000; }
  private:
    virtual const char* Name() const;
    virtual bool        trigger_CC1        () const;    // FALSE
    virtual unsigned    trigger_duration_us() const;
    virtual int PicPortCameraInit();
    virtual FrameHandle *PicPortFrameProcess(FrameHandle *pFrame);
    virtual unsigned output_resolution() const;
    virtual unsigned    pixel_rows         () const;
    virtual unsigned    pixel_columns      () const;
  private:
    unsigned long LastCount;
  public:
    unsigned long CurrentCount;
  private:
    const FccdConfigType* _inputConfig;
    char* _outputBuffer;
  };

}

#endif
