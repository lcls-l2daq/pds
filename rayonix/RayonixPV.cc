#include <string.h>
#include <stdio.h>

#include "pds/epicstools/PVWriter.hh"
#include "pds/service/Semaphore.hh"

#include "RayonixPV.hh"

using Pds_Epics::PVWriter;

using namespace Pds;

RayonixPV::RayonixPV(const char* pvbase, bool verbose) :
  _initialized(false),
  _verbose(verbose)
{
  strncpy(_pvName,pvbase,PVNAMELEN);
}

RayonixPV::~RayonixPV()
{
  if (_initialized) {
    for(unsigned i=0; i<NTEMPERATURES; i++)
      delete _valu_writer[i];
  }
}

void RayonixPV::initialize()
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

void RayonixPV::update(std::vector<double> temperature)
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
        printf("Error: PV '%s' not connected for Rayonix temperature #%u\n",
               _valu_writer_name[uu].c_str(), uu);
      }
    } else {
      printf("Error: no data recvd for Rayonix temperature #%u\n", uu);
    }
  }
}
