#ifndef Pds_EbKeyBank_hh
#define Pds_EbKeyBank_hh

#include "EbPulseId.hh"
#include "EbEvrSequence.hh"

#include <stdio.h>

namespace Pds {

  class EbKey {
  public:
    enum Type { PulseId, EVRSequence, NumberOfKeys };
  public:
    EbKey() : types(0) {}

    unsigned       types;
    EbPulseId      pulseId;
    EbEvrSequence  evrSequence;

    void print() const;
  };

}

inline void Pds::EbKey::print() const
{
  printf("pulseId: %08x/%08x\n", pulseId._data[1], pulseId._data[0]);
}

#endif
