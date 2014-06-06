#ifndef Pds_RayonixPV_hh
#define Pds_RayonixPV_hh

#include <vector>
#include <string>

#include "pds/epicstools/PVWriter.hh"
#include "pds/service/Semaphore.hh"

using Pds_Epics::PVWriter;

namespace Pds {
  class RayonixPV {
  public:
    enum { NTEMPERATURES=4 };
    enum { PVNAMELEN=32 };
    RayonixPV(const char* pvbase, bool verbose);
    ~RayonixPV();
  public:
    void    initialize();
    void    update(std::vector<double> temperature);

  private:
    char _pvName[PVNAMELEN];
    bool _initialized;
    PVWriter*  _valu_writer[NTEMPERATURES];
    std::string  _valu_writer_name[NTEMPERATURES];
    bool _verbose;
  };
};

#endif
