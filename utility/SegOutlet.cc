#include "pds/utility/SegOutlet.hh"

#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/collection/CollectionManager.hh"
#include "pds/xtc/InDatagram.hh"

#include <errno.h>

using namespace Pds;

SegOutlet::SegOutlet(Outlet& outlet,
                     CollectionManager& collection,
                     const Ins&         occurrences) :
  OutletWire  (outlet),
  _collection (collection),
  _client     (sizeof(Datagram), 0x1000000),
  _occurrences(occurrences)
{
}

SegOutlet::~SegOutlet()
{
}

Transition* SegOutlet::forward(Transition* tr)
{
  Ins dst(tr->reply_to());
  _collection.ucast(*tr,dst);
  return 0;
}

Occurrence* SegOutlet::forward(Occurrence* tr)
{
  if (tr->id() != OccurrenceId::EvrCommand)
    _collection.ucast(*tr,_occurrences);
  return 0;
}

InDatagram* SegOutlet::forward(InDatagram* dg)
{
  if (dg->datagram().seq.service()!=TransitionId::L1Accept) {
    _client.send((char*)dg, dg->xtc.payload(), dg->xtc.sizeofPayload(), _datagrams);
  }
  return 0;
}

void SegOutlet::bind(NamedConnection, const Ins& ins) 
{
  _datagrams = ins;
}

void SegOutlet::bind(unsigned id, const Ins& node) 
{
}

void SegOutlet::unbind(unsigned id) 
{
}

