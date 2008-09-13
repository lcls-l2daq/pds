//#include "odfClocktFix.hh"

#include "Browser.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagramIterator.hh"


using namespace Pds;

/*
** ++
**
**
** --
*/

Browser::Browser(const Datagram& dg, InDatagramIterator* iter, int depth) : 
  InXtcIterator(dg.xtc, iter), 
  _header(1),
  _depth(depth)
{
  const InXtc& xtc = dg.xtc;
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  unsigned* identity = (unsigned*) &(xtc.tag.identity());
  unsigned* contains = (unsigned*) &(xtc.tag.contains()); 
  printf(" sequence # %08X/%08X with environment 0x%08X service %d\n",
         dg.high(), dg.low(), dg.env.value(), dg.service()); 
  printf(" source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
	 xtc.src.pid(), xtc.src.did(), 
	 *identity, *contains, 
	 xtc.tag.extent(), xtc.damage.value());
  if (_depth < 0) _dumpBinaryPayload(xtc, iter);
  }

Browser::Browser(const InXtc& xtc, InDatagramIterator* iter, int depth) : 
  InXtcIterator(xtc, iter), 
  _header(1),
  _depth(depth)
{
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  unsigned* identity = (unsigned*) &(xtc.tag.identity());
  unsigned* contains = (unsigned*) &(xtc.tag.contains()); 
  printf(" source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
	 xtc.src.pid(), xtc.src.did(), 
	 *identity, *contains, 
	 xtc.tag.extent(), xtc.damage.value());
  if (_depth < 0) _dumpBinaryPayload(xtc, iter);
  }

/*
** ++
**
**
** --
*/

int Browser::process(const InXtc& inXtc, InDatagramIterator* iter)
  {
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  unsigned* identity = (unsigned*) &(inXtc.tag.identity());
  unsigned* contains = (unsigned*) &(inXtc.tag.contains()); 
  for (int i = _depth; i < 2; i++) printf("  ");
  printf("source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
	 inXtc.src.pid(), inXtc.src.did(),
	 *identity, *contains,
	 inXtc.tag.extent(), inXtc.damage.value());
  if (_depth && (inXtc.sizeofPayload() >= (int) sizeof(InXtc))){
    Browser browser(inXtc, iter, _depth - 1);
    return browser.iterate();
  }
  return _dumpBinaryPayload(inXtc, iter);
  }


int Browser::_dumpBinaryPayload(const InXtc& inXtc, InDatagramIterator* iter){
  int remaining = inXtc.sizeofPayload() >> 2;
  if(remaining)
    {
      if (remaining > 4) remaining = 4;
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
  return remaining<<2;
}

