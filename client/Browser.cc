//#include "odfClocktFix.hh"

#include "Browser.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/InDatagramIterator.hh"


using namespace Pds;


static bool _containsInXtc(TypeId type)
{
  return (type.value == TypeNum::Id_InXtc);
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
  printf(" sequence # %08X/%08X with environment 0x%08X service %d\n",
         dg.seq.high(), dg.seq.low(), dg.env.value(), dg.seq.service()); 
  printf(" source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 xtc.src.log(), xtc.src.phy(), 
	 xtc.contains.value, 
	 xtc.extent, xtc.damage.value());
  if (!_containsInXtc(xtc.contains))
    advance += _dumpBinaryPayload(xtc, iter);
  //  if (_depth < 0) advance = _dumpBinaryPayload(xtc, iter);
}

Browser::Browser(const InXtc& xtc, InDatagramIterator* iter, int depth, int& advance) : 
  InXtcIterator(xtc, iter), 
  _header(1),
  _depth(depth)
{
  // This construction of the identity and contains values is a sleazy
  // trick which lets us print their contents as a simple hex value. 
  //  printf(" %p source %04X/%04X, type/contains %x/%x, extent %d, damage 0x%X\n", 
  //	 &xtc, xtc.src.pid(), xtc.src.did(), 
  //	 *identity, *contains, 
  //	 xtc.tag.extent(), xtc.damage.value());
  if (!_containsInXtc(xtc.contains))
    advance += _dumpBinaryPayload(xtc, iter);
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
  for (int i = _depth; i < 2; i++) printf("  ");
  printf("source %08X/%08X, contains %x, extent %d, damage 0x%X\n", 
	 inXtc.src.log(), inXtc.src.phy(),
	 inXtc.contains.value,
	 inXtc.extent, inXtc.damage.value());
  if (_containsInXtc(inXtc.contains)) {
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

