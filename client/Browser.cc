//#include "odfClocktFix.hh"

#include "Browser.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagramIterator.hh"


using namespace Pds;


static bool _containsInXtc(unsigned type)
{
  return (type == TypeNumPrimary::Id_InXtcContainer);
}

/*
** ++
**
**
** --
*/

Browser::Browser(const Datagram& dg, InDatagramIterator* iter, int depth, int& advance) : 
  InXtcIterator(dg.xtc, iter), 
  _header(1),
  _depth(depth)
{
  const InXtc& xtc = dg.xtc;
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  unsigned* contains = (unsigned*) &(xtc.tag.contains()); 
  printf(" sequence # %08X/%08X with environment 0x%08X service %d\n",
         dg.high(), dg.low(), dg.env.value(), dg.service()); 
  printf(" source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.pid(), xtc.src.did(), 
	 *contains, 
	 xtc.tag.extent(), xtc.damage.value());
  if (!_containsInXtc(*contains))
    advance = _dumpBinaryPayload(xtc, iter);
  //  if (_depth < 0) advance = _dumpBinaryPayload(xtc, iter);
}

Browser::Browser(const InXtc& xtc, InDatagramIterator* iter, int depth, int& advance) : 
  InXtcIterator(xtc, iter), 
  _header(1),
  _depth(depth)
{
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  unsigned* contains = (unsigned*) &(xtc.tag.contains()); 
  //  printf(" %p source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
  //	 &xtc, xtc.src.pid(), xtc.src.did(), 
  //	 *identity, *contains, 
  //	 xtc.tag.extent(), xtc.damage.value());
  if (!_containsInXtc(*contains))
    advance = _dumpBinaryPayload(xtc, iter);
  //  if (_depth < 0) advance = _dumpBinaryPayload(xtc, iter);
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
  unsigned* contains = (unsigned*) &(inXtc.tag.contains()); 
  for (int i = _depth; i < 2; i++) printf("  ");
  printf("source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 inXtc.src.pid(), inXtc.src.did(),
	 *contains,
	 inXtc.tag.extent(), inXtc.damage.value());
  if (_containsInXtc(*contains)) {
    //  if (_depth && (inXtc.sizeofPayload() >= (int) sizeof(InXtc))){
    int advance=0;
    Browser browser(inXtc, iter, _depth - 1, advance);
    return (advance + browser.iterate());
  }
  return _dumpBinaryPayload(inXtc, iter);
  }


int Browser::_dumpBinaryPayload(const InXtc& inXtc, InDatagramIterator* iter){
  int advance = inXtc.sizeofPayload() >> 2;
  if(advance)
    {
      advance = advance < 4 ? advance : 4;
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

