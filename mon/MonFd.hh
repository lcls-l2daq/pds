#ifndef Pds_MonFD_HH
#define Pds_MonFD_HH

namespace Pds {
  class MonFd {
  public:
    virtual ~MonFd() {}
    virtual int fd() const = 0;
    virtual int processIo() = 0;
  };
};

#endif
