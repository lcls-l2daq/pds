#ifndef Pds_CameraDriver_hh
#define Pds_CameraDriver_hh

namespace Pds {

  class Appliance;
  class CameraBase;
  class FrameServer;
  class FrameServerMsg;
  class UserMessage;

  class CameraDriver {
  public:
    CameraDriver(CameraBase& c) : _camera(c) {}
    virtual ~CameraDriver() {}
  public:
    virtual int initialize(UserMessage*) = 0;
    virtual int start_acquisition(FrameServer&,
                                  Appliance&,
				  UserMessage*) = 0;
    virtual int stop_acquisition() = 0;
    //  Serial command interface
    virtual int SendCommand(char* szCommand, 
			    char* pszResponse, 
			    int   iResponseBufferSize) = 0;
    virtual int SendBinary (char* szCommand, 
			    int   iCommandLen,
			    char* pszResponse, 
			    int   iResponseBufferSize) = 0;
  public:
    CameraBase&  camera() { return _camera; }
  private:
    int _start();
    int _stop ();
  private:
    CameraBase& _camera;
    
  };

};

#endif
