#include "DamageBrowser.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {
  class MyIterator : public XtcIterator {
  public:
    MyIterator(const Xtc& xtc,
	       std::list<Xtc>& list) :
      XtcIterator(const_cast<Xtc*>(&xtc)),
      _list(list),
      _damage(0)
    {
    }
    ~MyIterator() {}
  public:
    int process(Xtc* pxtc)
    {
      Xtc& xtc = *pxtc;
      if (xtc.contains.id() == TypeId::Id_Xtc) {
	MyIterator iter(xtc,_list);
	iter.iterate();
	unsigned damage = xtc.damage.value() & ~iter.damage();
	if (damage) {
	  bool lFound=false;
	  for(std::list<Xtc>::iterator it=_list.begin(); it!=_list.end(); it++)
	    if (it->src == xtc.src) {
	      lFound = true;
	      break;
	    }
	  if (!lFound)
	    _list.push_back(Xtc(xtc.contains,xtc.src,damage));
	}
      }
      _damage |= xtc.damage.value();
      return 1;
    }
  public:
    unsigned damage() const { return _damage; }
  private:
    std::list<Xtc>& _list;
    unsigned        _damage;
  };
};

using namespace Pds;

DamageBrowser::DamageBrowser(const InDatagram& dg) 
{
  MyIterator iter(const_cast<InDatagram&>(dg).datagram().xtc,_damaged);
  iter.iterate();
}

DamageBrowser::~DamageBrowser()
{
}

const std::list<Xtc>& DamageBrowser::damaged() const
{
  return _damaged;
}

