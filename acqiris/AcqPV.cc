#include <string.h>
#include <stdio.h>

#include "pds/epicstools/PVWriter.hh"
#include "pds/service/Semaphore.hh"

#include "AcqPV.hh"

using Pds_Epics::PVWriter;

using namespace Pds;

AcqPV::AcqPV(const char* pvbase, bool verbose) :
  _initialized(false),
  _verbose(verbose)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

AcqPV::~AcqPV()
{
  if (_initialized) {
    for(unsigned i=0; i<NTEMPERATURES; i++)
      delete _valu_writer[i];
  }
}

void AcqPV::initialize()
{
  if (_verbose) {
    printf(" *** entered %s ***\n", __PRETTY_FUNCTION__);
  }

  char buff[64];
  for(int i=0; i<NTEMPERATURES; i++) {
    sprintf(buff,"%s:TEMPERATURE%d.VAL",_pvName,i);
    _valu_writer[i] = new PVWriter(buff);
    _valu_writer_name[i] = buff;
  }
  _initialized = true;
}

void AcqPV::update(std::vector<double> temperature)
{
  if (!_initialized) {
    printf("Error: %s called before initialize()\n", __PRETTY_FUNCTION__);
    return;
  }

  for (unsigned int uu=0; uu < NTEMPERATURES; uu++) {
    if (temperature.size() > uu) {
      *reinterpret_cast<double*>(_valu_writer[uu]->data()) = temperature.at(uu);
      if (_valu_writer[uu]->connected()) {
        _valu_writer[uu]->put();
      } else {
        printf("Error: PV '%s' not connected for Acq temperature #%u\n",
               _valu_writer_name[uu].c_str(), uu);
      }
    } else {
      printf("Error: no data recvd for Acq temperature #%u\n", uu);
    }
  }
}
