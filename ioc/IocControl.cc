#include "pds/ioc/IocControl.hh"
#include "pds/ioc/IocNode.hh"
#include "pds/ioc/IocHostCallback.hh"

using namespace Pds;

IocControl::IocControl() :
  _station(0),
  _expt_id(0)
{
  /**  Example list of IOCs [IP,port,phy,alias] **/
  /**  I don't know the source of this list     **/
  //  _nodes.push_back(new IocNode(0xac150a00,0,0x25000400,"XcsCam"));
}

IocControl::IocControl(const char* offlinerc,
		       const char* instrument,
		       unsigned    station,
		       unsigned    expt_id) :
  _offlinerc (offlinerc),
  _instrument(instrument),
  _station   (station),
  _expt_id   (expt_id)
{
  /**
     Example constructor parameters may be:
     offlinerc = '/reg/g/pcds/dist/pds/sxr/misc/.offlinerc'
     instrument = 'SXR'
     station    = 0 (sometimes 1 in CXI)
  **/

  /**  Example list of IOCs [IP,port,phy,alias] **/
  /**  I don't know the source of this list     **/
  //  _nodes.push_back(new IocNode(0xac150a00,0,0x25000400,"XcsCam"));
}

IocControl::~IocControl()
{
}

void IocControl::host_rollcall(IocHostCallback* cb)
{
  if (cb) {
    /**  Advertise all available IOCs for recording 
	 Can be done in a separate thread **/
    /**  Example callback **/
    for(std::list<IocNode*>::iterator it=_nodes.begin();
	it!=_nodes.end(); it++)
      cb->connected(*(*it));
  }
}

void IocControl::set_partition(const std::list<DetInfo>& iocs)
{
  _selected_nodes.clear();
  for(std::list<DetInfo>::const_iterator it=iocs.begin();
      it!=iocs.end(); it++)
    for(std::list<IocNode*>::iterator nit=_nodes.begin();
	nit!=_nodes.end(); nit++)
      if (it->phy() == (*nit)->src().phy())
	_selected_nodes.push_back(*nit);
}

Transition* IocControl::transitions(Transition* tr)
{
  switch(tr->id()) {
  case TransitionId::BeginRun:
    { unsigned run = tr->env().value();
      /**  Open the file for recording **/
    } break;
  case TransitionId::EndRun:
    /**  Close the file **/
    break;
  case TransitionId::Enable:
    /**  Start writing **/
    break;
  case TransitionId::Disable:
    /**  Stop writing **/
    break;
  default:
    break;
  }
  return tr;
}
