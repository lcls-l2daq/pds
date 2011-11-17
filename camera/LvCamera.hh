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

#include "FrameHandle.hh"

namespace PdsLeutron {

class LvCamera {
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
      NOTIFYTYPE_NONE,    // GetFrameHandle will return NULL if no frame is ready.
      NOTIFYTYPE_WAIT,    // GetFrame pause if no frame is ready.
      NOTIFYTYPE_SIGNAL,  // A signal will be generated when a new frame is ready.
      NOTIFYTYPE_POLL,    // Instead of a signal, uses a file descriptor poll method.
    };
    struct Status {
      char *CameraId;
      char *CameraName;
      unsigned long CapturedFrames;
      unsigned long DroppedFrames;
      enum LvCamera::State State;
    };

    //! Camera
    //! Constructor
    //! @return N/A
    LvCamera();

    //! Camera
    //! Destructor
    //! @return N/A
    ~LvCamera();

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
    int ConfigReset();

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

    //! GetStatus
    //! Return the current status of the camera.
    //! @param  Status  Camera::Status structure where the status will be stored.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    int GetStatus(struct Status &Status);

    //! GetFrameHandle
    //! Return the next available frame.
    //! @param  Status  Camera::Status structure where the status will be stored.
    //! @return 0 if successful, a negative value defined in errno.h
    //!         otherwise.
    FrameHandle *GetFrameHandle();

  protected:
    Status status;
};

}

#endif  // #ifndef PDS_CAMERA
