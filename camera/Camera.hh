//! Camera.hh
//! Camera class definition file. Most classes are virtual and must be implemented
//! by specific camera drivers classes that inherit of this base class.
//!
//! Copyright 2008, SLAC
//! Author: rmachet@slac.stanford.edu
//! GPL license
//!

#ifndef PDS_CAMERA
#define PDS_CAMERA

namespace Pds {

class Frame {
  public:
    enum Format {
      FORMAT_GRAYSCALE_8,
      FORMAT_GRAYSCALE_10,
      FORMAT_GRAYSCALE_12,
      FORMAT_GRAYSCALE_16,
      FORMAT_RGB_32,
    };

    Frame(unsigned long _width, unsigned long _height, enum Format _format, void *_data);
    Frame(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data);
    Frame(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data,
            void(*_release)(void *, Frame *, void *), void *_obj, void *_arg);
    ~Frame();
    unsigned long width;
    unsigned long height;
    enum Format format;
    unsigned long elsize;
    void *data;
    void(*release)(void *, Frame *pFrame, void *arg);
    void *obj;
    void *arg;
};

class Camera {
  public:
    enum Mode {
      MODE_CONTINUOUS,
      MODE_EXTTRIGGER,
      MODE_EXTTRIGGER_SHUTTER,
    };
    enum State {
      CAMERA_UNCONFIGURED,
      CAMERA_READY,
      CAMERA_RUNNING,
      CAMERA_STOPPED,
    };
    enum NotifyType {
      NOTIFYTYPE_NONE,    // GetFrame will return NULL if no frame is ready.
      NOTIFYTYPE_WAIT,    // GetFrame pause if no frame is ready.
      NOTIFYTYPE_SIGNAL,  // A signal will be generated when a new frame is ready.
      NOTIFYTYPE_POLL,    // Instead of a signal, uses a file descriptor poll method.
    };
    struct Status {
      char *CameraId;
      char *CameraName;
      unsigned long CapturedFrames;
      unsigned long DroppedFrames;
      enum Camera::State State;
    };
    struct Config {
      enum Camera::Mode Mode;
      enum Frame::Format Format;
      int GainPercent;
      int BlackLevelPercent;
      unsigned long ShutterMicroSec;  // Only applicable if Mode != MODE_EXTTRIGGER_SHUTTER
      int FramesPerSec;  // Only applicable if Mode == MODE_DEFAULT
    };

    //! Camera
    //! Constructor
    //! @return N/A
    Camera();

    //! Camera
    //! Destructor
    //! @return N/A
    ~Camera();

    //! SetNotification
    //! Select how the camera should notify the application that
    //! it has a new frame available (ie GetFrame will not block
    //! or return NULL).
    //! @param  mode  One of NotifyType enum
    //! @return a negative value defined in errno.h if an error occured
    //!         or teh requested mode is not supported by this Camera.
    //!         If successful the return value depends on mode:
    //!         NOTIFYTYPE_NONE: 0
    //!         NOTIFYTYPE_WAIT: 0
    //!         NOTIFYTYPE_SIGNAL: signal to use.
    //!         NOTIFYTYPE_POLL: file descriptor to use.
    //! @note   Must be called before Init() is called.
    int SetNotification(enum NotifyType mode);

    //! SetConfig
    //! Set the camera configuration that will be used next time
    //! Init() is called.
    //! @param  Config  Global configuration to use, a specific camera 
    //!                 may implement extra APIs to set camera specific
    //!                 parameters or do it through SendCommand().
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    //! @note   SetConfig is not expected to configure the camera, it 
    //!         should be configured when Init() is called. For this
    //!         reason the default implementation of this API should
    //!         suffice.
    int SetConfig(const struct Config &Config);

    //! Init
    //! Detect, configure and initialize the camera.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    //! @note   No configuration is expected to happen after Init() is
    //!         called and none shall enter in effect before Init() is
    //!         called again! Because of this rule a camera can be fully
    //!         initialized and ready to acquire after Init() has been 
    //!         called (to save time in Start().
    int Init();

    //! Start
    //! Start acquisition, after Start has been called the application
    //! must call GetFrame fast and often enough to avoid dropping frames.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    //! @note   Most of the setup and getting the camera ready should be
    //!         in Init(), Start() should be as fast as possible.
    int Start();

    //! Stop
    //! Stop acquisition. After that an application can call Init() again
    //! (if the configuration must change) or Start().
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    //! @note   If the application does not call Init() before calling Start()
    //!         the camera may take a little bit more time to start depending
    //!         on the driver and Camera itself.
    int Stop();

    //! GetConfig
    //! Return the configuration last set.
    //! @param  Config  Camera::Config structure where the current configuration
    //!                 will be stored.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    //! @note   This API only return the standard part of the configuration, see
    //!         the other SetConfig if you would like the whole configuration.
    int GetConfig(struct Config &Config);

    //! GetConfig
    //! Return the configuration last set, including static and camera specific fields.
    //! @param  Config  Camera::Config structure where the current configuration
    //!                 will be stored.
    //! @param  StaticConfigExtra   Pointer to a camera specific database that contains
    //!                             parameters that are camera specific or static
    //! @param  StaticConfigExtraSize Size of the memory area pointed to by StaticConfigExtra
    //!                               If too small, only StaticConfigExtraSize will be copied
    //!                               and this API will return the number of bytes required
    //!                               to get all the information.
    //! @return the size of StaticConfigExtra if successful, a negative value defined in errno.h
    //!         otherwise. If the return value is bigger that StaticConfigExtraSize, then the
    //!         content of StaticConfigExtra has been truncated.
    int GetConfig(struct Config &Config, void *StaticConfigExtra, int StaticConfigExtraSize);

    //! GetStatus
    //! Return the current status of the camera.
    //! @param  Status  Camera::Status structure where the status will be stored.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    int GetStatus(struct Status &Status);

    //! GetFrame
    //! Return the next available frame.
    //! @param  Status  Camera::Status structure where the status will be stored.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    Frame *GetFrame();

    //! SendCommand
    //! Send a camera specific command to the camera.
    //! @param  szCommand  Zero terminated string that contains the command.
    //! @param  pszResponse         Memory area where any response may be stored.
    //! @param  iResponseBufferSize Maximum size of the response, the API will  
    //!                             change this value to the exact size
    //!                             of the response.
    //! @return number of bytes written in pszResponse if successful, a negative 
    //!         value defined in errno.h otherwise.
    //! @note   SendCommand can only be used AFTER Init()
    int SendCommand(char *szCommand, char *pszResponse, int iResponseBufferSize);

  protected:
    Status status;
    Config config;
    void *pExtraConfig;
    int lenExtraConfig;
  private:

};

}

#endif  // #ifndef PDS_CAMERA