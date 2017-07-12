#include "pds/client/Browser.hh"
#include "pds/xtc/Datagram.hh"

#include <stdio.h>

using namespace Pds;


static bool _containsXtc(TypeId type)
{
  return (type.id() == TypeId::Id_Xtc);
}

/*
** ++
**
**
** --
*/

Browser::Browser(const Datagram& dg, int depth) : 
  XtcIterator(const_cast<Xtc*>(&dg.xtc)), 
  _header(1),
  _depth(depth)
{
  const Xtc& xtc = dg.xtc;
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  printf(" sequence # %016lx with environment 0x%08X service %d\n",
         dg.seq.stamp().fiducials(),
	 dg.env.value(), dg.seq.service()); 
  printf(" source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.log(), xtc.src.phy(), 
	 xtc.contains.value(), 
	 xtc.extent, xtc.damage.value());
}

Browser::Browser(const Xtc& xtc, int depth) : 
  XtcIterator(const_cast<Xtc*>(&xtc)), 
  _header(1),
  _depth(depth)
{
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  //  printf(" %p source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
  //	 &xtc, xtc.src.pid(), xtc.src.did(), 
  //	 *identity, *contains, 
  //	 xtc.tag.extent(), xtc.damage.value());
}

/*
** ++
**
**
** --
*/

int Browser::process(Xtc* pxtc)
  {
    const Xtc& xtc = *pxtc;
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  for (int i = _depth; i < 2; i++) printf("  ");
  printf("source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.log(), xtc.src.phy(),
	 xtc.contains.value(),
	 xtc.extent, xtc.damage.value());
  if (_containsXtc(xtc.contains)) {
    //  if (_depth && (xtc.sizeofPayload() >= (int) sizeof(Xtc))){
    Browser browser(xtc, _depth - 1);
    (browser.iterate());
    return 1;
  }
  return _dumpBinaryPayload(xtc);
  }


static int DumpWords=4;

void Browser::setDumpLength(unsigned l) { DumpWords = l>>2; }

int Browser::_dumpBinaryPayload(const Xtc& xtc){
  int advance = xtc.sizeofPayload() >> 2;
  if(advance)
    {
      //      const int DumpWords=10;
      advance = advance < DumpWords ? advance : DumpWords;
      int remaining = advance;

      unsigned* next = (unsigned*)xtc.payload();
      unsigned  value;
      do 
	{
	  value = *next++;
	  printf("        Contents are 0x%08X\n", value);
	}
      while(--remaining);
    }
  return advance<<2;
}

