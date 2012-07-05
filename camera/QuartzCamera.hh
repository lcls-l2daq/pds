#ifndef Pds_Quartz4ACamera_hh
#define Pds_Quartz4ACamera_hh

#include "pds/camera/CameraBase.hh"
#include "pds/config/QuartzConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {
  class QuartzCamera: public CameraBase {
  public:
    QuartzCamera(const DetInfo& src);  // identifies Quartz model (2A340, 4A150, 4A180)
    ~QuartzCamera();
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
    const DetInfo _src;
    const QuartzConfigType* _inputConfig;
  };
}

#endif
