//! Opal1000.hh
//! Opal1000 camera control class. This class inherits of PicPortCL, which
//! means it assumes the Camera is connected to a Leutron frame grabber.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#ifndef PDS_CAMERA_OPAL1000
#define PDS_CAMERA_OPAL1000

#include "PicPortCL.hh"

#define OPAL1000_NAME             "Adimec Opal 1000"
#define OPAL1000_NAME_12bits      "Adimec_Opal-1000m/Q"
#define OPAL1000_NAME_10bits      "Adimec_Opal-1000m/Q_F10bit"
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

namespace Pds {

  class Opal1000: public PicPortCL {
  public:
    Opal1000(char *id = NULL);
    virtual ~Opal1000();
  protected:
    virtual int PicPortCameraConfig(LvROI &Roi);
    virtual int PicPortCameraInit();
    virtual Pds::Frame *PicPortFrameProcess(Pds::Frame *pFrame);
  private:
    unsigned long LastCount;
  public:
    unsigned long CurrentCount;
  };

}

#endif	// #ifndef PDS_CAMERA_OPAL1000
