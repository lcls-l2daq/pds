//! Opal1kCamera.hh
//! Opal1kCamera camera control class. This class inherits of PicPortCL, which
//! means it assumes the Camera is connected to a Leutron frame grabber.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#ifndef Pds_Opal1kCamera_hh
#define Pds_Opal1kCamera_hh

#include "pds/camera/PicPortCL.hh"
#include "pds/config/Opal1kConfigType.hh"

#define OPAL1000_NAME             "Adimec Opal 1000"
#define OPAL1000_NAME_12bits      "Adimec_Opal-1000m/Q"
#define OPAL1000_NAME_10bits      "Adimec_Opal-1000m/Q_F10bits"
#define OPAL1000_NAME_8bits       "Adimec_Opal-1000m/Q_F8bit"


namespace PdsLeutron {

  class Opal1kCamera : public PicPortCL {
  public:
    Opal1kCamera(char* id, unsigned grabberId=0, const char *grabberName = "Mono");
    virtual ~Opal1kCamera();

    void                    Config(const Opal1kConfigType&);
    int                     setTestPattern( bool on );
    //    const Opal1kConfigType& Config() const;

  private:
    //  Serial command interface
    virtual int           baudRate() const { return 57600; }
    virtual unsigned long parity  () const { return LvComm_ParityNone; }
    virtual unsigned long byteSize() const { return LvComm_Data8; }
    virtual unsigned long stopSize() const { return LvComm_Stop1; }
    virtual char          eotWrite() const { return 0x06; }
    virtual char          eotRead () const { return '\r'; }
    virtual char          sof     () const { return '@'; }
    virtual char          eof     () const { return '\r'; }
    virtual unsigned long timeout_ms() const { return 1000; }
  private:
    virtual const char* Name() const;
    virtual bool        trigger_CC1        () const;
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
    const Opal1kConfigType* _inputConfig;
  };

}

#endif
