//! TM6740Camera.hh
//! TM6740Camera camera control class. This class inherits of PicPortCL, which
//! means it assumes the Camera is connected to a Leutron frame grabber.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#ifndef Pds_TM6740Camera_hh
#define Pds_TM6740Camera_hh

#include "pds/camera/PicPortCL.hh"
#include "pds/config/TM6740ConfigType.hh"

#define TM6740_NAME             "Pulnix TM6740CL"
#define TM6740_NAME_10bits      "Pulnix_TM6740CL_10bit"
#define TM6740_NAME_8bits       "Pulnix_TM6740CL_8bit"


namespace PdsLeutron {

  class TM6740Camera : public PicPortCL {
  public:
    TM6740Camera(char* id, unsigned grabberId=0, const char *grabberName = "Mono");
    virtual ~TM6740Camera();

    void                    Config(const TM6740ConfigType&);
    //    const TM6740ConfigType& Config() const;
  private:
    //  Serial command interface
    virtual int           baudRate() const { return 9600; }
    virtual unsigned long parity  () const { return LvComm_ParityNone; }
    virtual unsigned long byteSize() const { return LvComm_Data8; }
    virtual unsigned long stopSize() const { return LvComm_Stop1; }
    virtual char          eotWrite() const { return '\r'; }
    virtual char          eotRead () const { return '\r'; }
    virtual char          sof     () const { return ':'; }
    virtual char          eof     () const { return '\r'; }
    virtual unsigned long timeout_ms() const { return 1000; }
  private:
    virtual const char* Name() const;
    virtual bool        trigger_CC1        () const;
    virtual unsigned    trigger_duration_us() const;
    virtual int PicPortCameraInit();
    virtual unsigned output_resolution() const;
    virtual unsigned    pixel_rows         () const;
    virtual unsigned    pixel_columns      () const;
  private:
    const TM6740ConfigType* _inputConfig;
  };

}

#endif
