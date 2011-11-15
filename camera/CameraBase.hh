#ifndef Pds_CameraBase_hh
#define Pds_CameraBase_hh

namespace Pds {

  class CameraDriver;
  class FrameServerMsg;
  class UserMessage;

  class CameraBase {
  public:
    CameraBase();
    virtual ~CameraBase();
  public:
    virtual void set_config_data(const void*) = 0;
    virtual int  configure      (CameraDriver&,
				 UserMessage*) = 0;
  public:
    virtual int           baudRate() const=0;
    virtual char          eotWrite() const=0;
    virtual char          eotRead() const=0;
    virtual char          sof() const=0;
    virtual char          eof() const=0;
    virtual unsigned long timeout_ms() const=0;
  public:
    virtual bool        validate(FrameServerMsg&) = 0;
    virtual int         camera_width () const = 0;
    virtual int         camera_height() const = 0;
    virtual int         camera_depth () const = 0;
    virtual int         camera_taps  () const = 0;
    virtual const char* camera_name  () const = 0; // Leutron camera lookup
  };

};

#endif
