#ifndef Pds_CmdLineTools_hh
#define Pds_CmdLineTools_hh

#include <stdint.h>

namespace Pds {
  class DetInfo;
  class CmdLineTools {
  public:
    static bool parseDetInfo(const char* args, DetInfo& info);
    static bool parseInt   (const char* arg, int&, int base=0);
    static bool parseUInt  (const char* arg, unsigned&, int base=0);
    static int  parseUInt  (      char* arg, unsigned&, unsigned&, unsigned&, int base=0, int delim=',');
    static bool parseUInt64(const char* arg, uint64_t&, int base=0);
    static bool parseFloat (const char* arg, float&);
    static bool parseDouble(const char* arg, double&);
    static bool parseSrcAlias(const char* arg);
  };
};

#endif
