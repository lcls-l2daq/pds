#ifndef Pds_TM6740Camera_hh
#define Pds_TM6740Camera_hh

#include "pds/camera/CameraBase.hh"
#include "pds/config/TM6740ConfigType.hh"

namespace Pds {

  class TM6740Camera: public CameraBase {
  public:
    TM6740Camera();
    ~TM6740Camera();
  public:
    void set_config_data  ( const void*);
    int  setContinuousMode( CameraDriver&, double fps );
  private:
    bool validate(Pds::FrameServerMsg&);
    int  configure(CameraDriver&,
		   UserMessage*);
    int  camera_width () const;
    int  camera_height() const;
    int  camera_depth () const;
    int  camera_taps  () const;
    const char* camera_name() const;
  protected:
    //  Serial command interface
    int           baudRate() const;
    char          eotWrite() const;
    char          eotRead() const;
    char          sof() const;
    char          eof() const;
    unsigned long timeout_ms() const;
  private:
    const TM6740ConfigType* _inputConfig;
  };
}

#endif
