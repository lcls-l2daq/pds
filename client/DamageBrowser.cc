#include "DamageBrowser.hh"

#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/CDatagramIterator.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/service/GenericPool.hh"

namespace Pds {
  class MyIterator : public PdsClient::XtcIterator {
  public:
    MyIterator(const Xtc& xtc,
	       InDatagramIterator* it,
	       std::list<Xtc>& list) :
      XtcIterator(xtc, it),
      _list(list),
      _damage(0)
    {
    }
    ~MyIterator() {}
  public:
    int process(const Xtc& xtc, InDatagramIterator* it)
    {
      int advance = 0;
      if (xtc.contains.id() == TypeId::Id_Xtc) {
	MyIterator iter(xtc,it,_list);
	advance += iter.iterate();
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
      return advance;
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
  GenericPool* pool = new GenericPool(sizeof(CDatagramIterator),2);
  MyIterator iter(dg.datagram().xtc, dg.iterator(pool),_damaged);
  iter.iterate();
}

DamageBrowser::~DamageBrowser()
{
}

const std::list<Xtc>& DamageBrowser::damaged() const
{
  return _damaged;
}

