#include "EbSGroup.hh"

#include "EbEvent.hh"
#include "EbSequenceKey.hh"
#include "pds/vmon/VmonEb.hh"

using namespace Pds;

EbSGroup::EbSGroup(const Src& id,
   const TypeId& ctns,
   Level::Type level,
   Inlet& inlet,
   OutletWire& outlet,
   int stream,
   int ipaddress,
   unsigned eventsize,
   unsigned eventpooldepth,
   int slowEb,
   VmonEb* vmoneb) :
  EbS(id, ctns, level, inlet, outlet,
     stream, ipaddress,
     eventsize, eventpooldepth, slowEb, vmoneb)
{
}

EbSGroup::~EbSGroup()
{
}

EbBase::IsComplete EbSGroup::_is_complete( EbEventBase* event, const EbBitMask& serverId)
{
  Datagram& dg = *event->datagram();
  TransitionId::Value service = dg.seq.service();

  //!!!debug
  //if (service == TransitionId::L1Accept)
  //{
  //  printf("EbSGroup::_is_complete(): Sequence 0x%x\n", dg.seq.stamp().fiducials());
  //  if (event->isClientGroupSet())
  //    printf("EbSGroup::_is_complete(): Client Group has been set\n");
  //}

  if (service == TransitionId::L1Accept && !event->isClientGroupSet())
  {
    uint32_t uClientGroupMask = ((L1AcceptEnv&) dg.env).clientGroupMask();

    EbBitMask maskClientGroup;
    if (_lGroupClientMask.size() == 0)
      new (&maskClientGroup) EbBitMask(EbBitMask::FULL);
    else
    {
      uint32_t uGroupBit = 1;
      for (int iGroup = 0; iGroup < (int) _lGroupClientMask.size(); ++iGroup, uGroupBit <<=1)
      {
        if (uClientGroupMask & uGroupBit)
          maskClientGroup |= _lGroupClientMask[iGroup];
      }
    }

    EbBitMask remain1 = event->remaining();
    event->setClientGroup(maskClientGroup);
    EbBitMask remain2 = event->remaining();

    //!!!debug
    //printf( "EbSGroup::_is_complete(): service 0x%x group 0x%x server 0x%x Client 0x%x remaining 0x%x -> 0x%x\n",
    //  dg.seq.service(), uClientGroupMask,
    //  serverId.value(0),
    //  maskClientGroup.value(0),
    //  remain1.value(0),
    //  remain2.value(0)
    //  );
  }

  return EbS::_is_complete(event, serverId);
}
