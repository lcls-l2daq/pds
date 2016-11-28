#include "pds/quadadc/Adt7411.hh"

using namespace Pds::QuadAdc;

#define REG(r) (_reg[r]&0xff)

unsigned Adt7411::deviceId       () const { return REG(0x4d); }
unsigned Adt7411::manufacturerId () const { return REG(0x4e); }
unsigned Adt7411::interruptStatus() const { return REG(0x00) & (REG(0x01)<<8); }
unsigned Adt7411::interruptMask  () const { return REG(0x1d) & (REG(0x1e)<<8); }
unsigned Adt7411::internalTemp   () const { return REG(0x03) & (REG(0x07)<<8); }
unsigned Adt7411::externalTemp   () const { return REG(0x04) & (REG(0x08)<<8); }

void Adt7411::interruptMask(unsigned m)
{
  _reg[0x1d] = m&0xff;
  _reg[0x1e] = (m>>8)&0xff;
}
