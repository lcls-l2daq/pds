#ifndef Pds_FrameHandle_hh
#define Pds_FrameHandle_hh

namespace PdsLeutron {

  class FrameHandle {
  public:
    enum Format {
      FORMAT_GRAYSCALE_8,
      FORMAT_GRAYSCALE_10,
      FORMAT_GRAYSCALE_12,
      FORMAT_GRAYSCALE_16,
      FORMAT_RGB_32,
    };

    FrameHandle(unsigned long _width, unsigned long _height, enum Format _format, void *_data);
    FrameHandle(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data);
    FrameHandle(unsigned long _width, unsigned long _height, enum Format _format, int _elsize, void *_data,
		void(*_release)(void *, FrameHandle *, void *), void *_obj, void *_arg);
    ~FrameHandle();
    unsigned long width;
    unsigned long height;
    enum Format format;
    unsigned long elsize;
    void *data;
    void(*release)(void *, FrameHandle *pFrameHandle, void *arg);
    void *obj;
    void *arg;

  public:
    unsigned depth() const;
  };

};

#endif
