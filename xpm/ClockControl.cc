#include "ClockControl.hh"

#include <unistd.h>
#include <stdio.h>

using namespace Pds::Xpm;
using Pds::Cphw::Reg;

ClockControl::ClockControl()
{ dump(); }

void ClockControl::dump()
{
  printf("ClockControl\n");
  printf("devType   %x\n", unsigned(_deviceType));
  printf("prodUpper %x\n", unsigned(_prodUpper));
  printf("prodLower %x\n", unsigned(_prodLower));
  printf("vndrUpper %x\n", unsigned(_vndrUpper));
  printf("vndrLower %x\n", unsigned(_vndrLower));
}

void ClockControl::init()
{
  _lmkReg[0x38]=0x40; // take CLKin1 directly to outputs
  _lmkReg[0x47]=0x32;
  _lmkReg[0x03]=0x02; // Bypass dividers
  _lmkReg[0x0b]=0x02;
  _lmkReg[0x13]=0x02;
  _lmkReg[0x1b]=0x02;
  _lmkReg[0x23]=0x02;
  _lmkReg[0x2b]=0x02;
  _lmkReg[0x33]=0x02;

  _lmkReg[0x18]=0x01;
  _lmkReg[0x20]=0x01;
  _lmkReg[0x28]=0x01;
  _lmkReg[0x30]=0x01;

  _lmkReg[0x18]=0x01;
  _lmkReg[0x20]=0x01;
  _lmkReg[0x28]=0x01;
  _lmkReg[0x30]=0x01;

  _lmkReg[0x1e]=0x70; // 
  _lmkReg[0x26]=0x70;
  _lmkReg[0x2e]=0x70;
  _lmkReg[0x36]=0x70;

  _lmkReg[0x1f]=0x11; // set DClkOut,SDClkOut (LVDS)
  _lmkReg[0x27]=0x11;
  _lmkReg[0x2f]=0x11;
  _lmkReg[0x37]=0x11;

  _lmkReg[0x40]=0xf1;
  _lmkReg[0x47]=0x43;
  _lmkReg[0x73]=0x60;

  _lmkClkSel=1;        // PLL1 unused, though
  _lmkMuxSel=1;        // Select SYS_CLK
  //_lmkMuxSel=0; 
}
