#ifndef Pds_Opal1kCamera_hh
#define Pds_Opal1kCamera_hh

#include "pds/camera/CameraBase.hh"
#include "pds/config/Opal1kConfigType.hh"

namespace Pds {

  class Opal1kCamera: public CameraBase {
  public:
    Opal1kCamera();
    ~Opal1kCamera();
  public:
    void set_config_data  ( const void*);
    int  setTestPattern   ( CameraDriver&, bool );
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
    uint32_t LastCount;
  public:
    uint32_t CurrentCount;
  private:
    const Opal1kConfigType* _inputConfig;
  };
}

#endif
