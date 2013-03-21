//#include "odfClocktFix.hh"

#include "Browser.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagramIterator.hh"

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

Browser::Browser(const Datagram& dg, InDatagramIterator* iter, int depth, int& advance) : 
  XtcIterator(dg.xtc, iter), 
  _header(1),
  _depth(depth)
{
  const Xtc& xtc = dg.xtc;
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  printf(" sequence # %08X/%08X/%02X with environment 0x%08X service %d\n",
         dg.seq.stamp().fiducials(), dg.seq.stamp().ticks(), dg.seq.stamp().control(),
	 dg.env.value(), dg.seq.service()); 
  printf(" source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.log(), xtc.src.phy(), 
	 xtc.contains.value(), 
	 xtc.extent, xtc.damage.value());
  if (!_containsXtc(xtc.contains))
    advance += _dumpBinaryPayload(xtc, iter);
  //  if (_depth < 0) advance = _dumpBinaryPayload(xtc, iter);
}

Browser::Browser(const Xtc& xtc, InDatagramIterator* iter, int depth, int& advance) : 
  XtcIterator(xtc, iter), 
  _header(1),
  _depth(depth)
{
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  //  printf(" %p source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
  //	 &xtc, xtc.src.pid(), xtc.src.did(), 
  //	 *identity, *contains, 
  //	 xtc.tag.extent(), xtc.damage.value());
  if (!_containsXtc(xtc.contains))
    advance += _dumpBinaryPayload(xtc, iter);
  }

/*
** ++
**
**
** --
*/

int Browser::process(const Xtc& xtc, InDatagramIterator* iter)
  {
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  for (int i = _depth; i < 2; i++) printf("  ");
  printf("source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.log(), xtc.src.phy(),
	 xtc.contains.value(),
	 xtc.extent, xtc.damage.value());
  if (_containsXtc(xtc.contains)) {
    //  if (_depth && (xtc.sizeofPayload() >= (int) sizeof(Xtc))){
    int advance=0;
    Browser browser(xtc, iter, _depth - 1, advance);
    return (advance + browser.iterate());
  }
  return _dumpBinaryPayload(xtc, iter);
  }


static int DumpWords=4;

void Browser::setDumpLength(unsigned l) { DumpWords = l>>2; }

int Browser::_dumpBinaryPayload(const Xtc& xtc, InDatagramIterator* iter){
  int advance = xtc.sizeofPayload() >> 2;
  if(advance)
    {
      //      const int DumpWords=10;
      advance = advance < DumpWords ? advance : DumpWords;
      int remaining = advance;
      iovec iov[1];
      int rlen = iter->read(iov,1,remaining<<2);
      if (rlen != remaining<<2)
	return rlen;

      unsigned* next = (unsigned*)iov[0].iov_base;
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

