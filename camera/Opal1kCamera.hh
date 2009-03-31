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
//#define OPAL1000_SERIAL_BAUDRATE  115200
#define OPAL1000_SERIAL_BAUDRATE  57600
#define OPAL1000_SERIAL_PARITY    LvComm_ParityNone
#define OPAL1000_SERIAL_DATASIZE  LvComm_Data8
#define OPAL1000_SERIAL_STOPSIZE  LvComm_Stop1
#define OPAL1000_SERIAL_ACK       0x06
#define OPAL1000_SERIAL_EOT       '\r'
#define OPAL1000_SERIAL_SOF       '@'
#define OPAL1000_SERIAL_TIMEOUT   1000
#define OPAL1000_CONNECTOR        "CamLink Base Port 0 (HVPSync In 0)"


namespace PdsLeutron {

  class Opal1kCamera : public PicPortCL {
  public:
    Opal1kCamera(char *id = NULL);
    virtual ~Opal1kCamera();

    void                    Config(const Opal1kConfigType&);
    const Opal1kConfigType& Config() const;
  protected:
    virtual int PicPortCameraConfig(LvROI &Roi);
    virtual int PicPortCameraInit();
    virtual FrameHandle *PicPortFrameProcess(FrameHandle *pFrame);
  private:
    unsigned long LastCount;
  public:
    unsigned long CurrentCount;
  private:
    const Opal1kConfigType* _inputConfig;
    char* _outputBuffer;
  };

}

#endif
