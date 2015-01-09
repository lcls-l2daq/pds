#ifndef Pds_AdimecCommander_hh
#define Pds_AdimecCommander_hh

#include <ndarray/ndarray.h>
#include <stdint.h>

namespace Pds {
  class CameraDriver;
  class AdimecCommander {
  public:
    AdimecCommander(CameraDriver& _driver);
  public:
    int setCommand(const char* title,
                   const char* cmd);
    void getParameter(const char* cmd, 
                      unsigned& val1);
    void getParameters(const char* cmd,
                       unsigned* valn,
                       unsigned n);
    int setParameter(const char* title,
                     const char* cmd,
                     unsigned    val1);
    int setParameter(const char* title,
                     const char* cmd,
                     unsigned* valn,
                     unsigned n);
    int setParameter(const char* cmd,
                     const ndarray<const uint16_t,1>& a);
  public:
    const char* response() const { return szResponse; }
  private:
    CameraDriver& driver;
    enum { SZCOMMAND_MAXLEN=64 };
    char szCommand [SZCOMMAND_MAXLEN];
    char szResponse[SZCOMMAND_MAXLEN];
  };
};

#endif
