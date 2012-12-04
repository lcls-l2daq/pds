#ifndef Pds_OrcaFlash4_0Camera_hh
#define Pds_OrcaFlash4_0Camera_hh

#include "pds/camera/CameraBase.hh"
#include "pds/config/OrcaConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

namespace Pds {
  class OrcaCamera: public CameraBase {
  public:
    OrcaCamera(const DetInfo& src);  // identifies Orca model (Flash4_0)
    ~OrcaCamera();
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
    int  camera_cfg2  () const;
    const char* camera_name() const;
    bool uses_sof     () const;
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
    const OrcaConfigType* _inputConfig;
  };
}

#endif
