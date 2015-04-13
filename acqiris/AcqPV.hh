#ifndef Pds_AcqPV_hh
#define Pds_AcqPV_hh

#include <vector>
#include <string>

#include "pds/epicstools/PVWriter.hh"
#include "pds/service/Semaphore.hh"

using Pds_Epics::PVWriter;

namespace Pds {
  class AcqPV {
  public:
    enum { NTEMPERATURES=5 };
    enum { PVNAMELEN=32 };
    AcqPV(const char* pvbase, bool verbose);
    ~AcqPV();
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
