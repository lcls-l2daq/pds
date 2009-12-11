#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include <new>
#include <vector>
#include <time.h>

#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Client.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/client/Fsm.hh"
#include "pds/client/Action.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/utility/StreamPorts.hh"
#include "RceProxyManager.hh"
#include "RceProxyMsg.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/config/pnCCDConfigType.hh"

using namespace Pds;
using std::string;
using std::vector;

class RceProxyAllocAction : public Action 
{
  public:
    RceProxyAllocAction(RceProxyManager& manager, CfgClientNfs& cfg) : _manager(manager), _cfg(cfg) {}

    virtual Transition* fire(Transition* tr)     
    {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _cfg.initialize(alloc.allocation());
      _manager.onActionMap(alloc.allocation());
      return tr;
    }
  private:
    RceProxyManager& _manager;
    CfgClientNfs& _cfg;
};

class RceProxyUnmapAction : public Action
{
  public:
    RceProxyUnmapAction(RceProxyManager& manager) : _manager(manager) {}

    virtual Transition* fire(Transition* tr)
    {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _manager.onActionUnmap(alloc.allocation());
      return tr;
    }
  private:
    RceProxyManager& _manager;
};

class RceProxyConfigAction : public Action 
{
  public:
    RceProxyConfigAction(RceProxyManager& manager, const Src& src, CfgClientNfs& cfg, int iDebugLevel, unsigned iNumLinks, unsigned iPayloadSizePerLink) :
      _manager(manager), _src(src), _cfg(cfg), _iDebugLevel(iDebugLevel),
      _cfgtc(_pnCCDConfigType,cfg.src()),
      _iNumLinks(iNumLinks),_iPayloadSizePerLink(iPayloadSizePerLink), _damageFromRce(0)
      {}

    // this is the first "phase" of a transition where
    // all CPUs configure in parallel.
    virtual Transition* fire(Transition* tr) 
    {
      // in the long term, we should get this from database - cpo
      //_cfg.fetch(*tr,_epicsArchConfigType, &_config);
      int iFail = _manager.onActionConfigure(&_damageFromRce);
      if ( iFail != 0 )
        _damageFromRce.increase(Damage::UserDefined);
      return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
      pnCCDConfigType config(_iNumLinks,_iPayloadSizePerLink);
      _cfgtc.extent = sizeof(Xtc)+sizeof(pnCCDConfigType);
      in->insert(_cfgtc, &config);
      
      if ( _damageFromRce.value() != 0 )
        in->datagram().xtc.damage.increase(_damageFromRce.value());
        
      return in;
    }

  private:
    //RceProxyConfigType _config;
    RceProxyManager&    _manager;
    Src                 _src;
    CfgClientNfs&       _cfg;
    int                 _iDebugLevel;
    Xtc                 _cfgtc;
    unsigned            _iNumLinks;
    unsigned            _iPayloadSizePerLink;
    Damage              _damageFromRce;
};

class RceProxyL1AcceptAction : public Action 
{
  public:
    RceProxyL1AcceptAction(RceProxyManager& manager, int iDebugLevel) :
      _manager(manager), _iDebugLevel(iDebugLevel)
    {
    }

    ~RceProxyL1AcceptAction()
    {}

    virtual InDatagram* fire(InDatagram* in)     
    {
      // Stop the propagation of L1Accept datagram for RceProxy
      // The real L1 Data will be sent from RCE
      return NULL;
    }

  private:
    RceProxyManager&   _manager;
    float               _fMinTriggerInterval;
    int                 _iDebugLevel;
};

class RceProxyDisableAction : public Action 
{
  public:
    RceProxyDisableAction(RceProxyManager& manager) : _manager(manager)
    {}

    virtual Transition* fire(Transition* in) 
    {
      return in;
    }
  private:
    RceProxyManager& _manager;
};

const Src RceProxyManager::srcLevel = Src(Level::Source);
RceFBld::ProxyMsg RceProxyManager::_msg;

RceProxyManager::RceProxyManager(CfgClientNfs& cfg, const string& sRceIp, int iNumLinks, int iPayloadSizePerLink, 
    TypeId typeidData, const Node& selfNode, int iDebugLevel) :
    _sRceIp(sRceIp), _iNumLinks(iNumLinks), _iPayloadSizePerLink(iPayloadSizePerLink), _typeidData(typeidData),
    _selfNode(selfNode), _iDebugLevel(iDebugLevel), _cfg(cfg)
{
  _pFsm            = new Fsm();
  _pActionMap      = new RceProxyAllocAction(*this, cfg);
  _pActionUnmap    = new RceProxyUnmapAction(*this);
  _pActionConfig   = new RceProxyConfigAction(*this, RceProxyManager::srcLevel, cfg, _iDebugLevel,_iNumLinks,iPayloadSizePerLink);
  // this should go away when we get the configuration from the database - cpo
  _pActionL1Accept = new RceProxyL1AcceptAction(*this, _iDebugLevel);
  _pActionDisable  = new RceProxyDisableAction(*this);

  _pFsm->callback(TransitionId::Map,        _pActionMap);
  _pFsm->callback(TransitionId::Unmap,      _pActionUnmap);
  _pFsm->callback(TransitionId::Configure,  _pActionConfig);
  _pFsm->callback(TransitionId::L1Accept,   _pActionL1Accept);
  _pFsm->callback(TransitionId::Disable,    _pActionDisable);
}

RceProxyManager::~RceProxyManager()
{
  delete _pActionDisable;
  delete _pActionL1Accept;
  delete _pActionConfig;
  delete _pActionMap;

  delete _pFsm;
}

int RceProxyManager::onActionConfigure(Damage* pDamageFromRce) 
{
  printf("RceProxy Configure transition\n");
  printf( "Sent %d bytes to RCE %s/%d NumLink %d PayloadSizePerLink 0x%x\n", sizeof(_msg), _sRceIp.c_str(),  RceFBld::ProxyMsg::ProxyPort,
      _iNumLinks, _iPayloadSizePerLink);
  DetInfo& detInfo = (DetInfo&) _msg.detInfoSrc;
  printf( "Detector %s Id %d  Device %s Id %d\n", DetInfo::name( detInfo.detector() ), detInfo.detId(),
      DetInfo::name( detInfo.device() ), detInfo.devId() );      
  
  int iSocket = socket(AF_INET, SOCK_DGRAM, 0);   
  if ( iSocket == -1 ) 
  {
    printf( "RceProxyManager::onActionConfigure(): socket() failed\n" );
    return 1;
  }

  sockaddr_in sockaddrServer;
  sockaddrServer.sin_family      = AF_INET;
  sockaddrServer.sin_addr.s_addr = inet_addr(_sRceIp.c_str());
  sockaddrServer.sin_port        = htons(RceFBld::ProxyMsg::ProxyPort);
  
  int iSizeSockAddr = sizeof(sockaddr_in);    
  int iStatus = sendto( iSocket, (char*) &_msg, sizeof(_msg), 0, (struct sockaddr*)&sockaddrServer, iSizeSockAddr );
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::onActionConfigure(): sendto() failed\n" );
    return 2;
  }
  
  timeval timeout = { 2, 0 }; // timeout in 2 secs
  
  fd_set  fdsetRead;
  FD_ZERO(&fdsetRead);
  FD_SET(iSocket, &fdsetRead);
  iStatus = select(iSocket+1, &fdsetRead, NULL, NULL, &timeout);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::onActionConfigure(): select() failed, %s\n", strerror(errno) );
    return 3;
  }
  else if ( iStatus == 0 ) // No socket is ready within the timeout
  {
    close(iSocket);
    printf( "RceProxyManager::onActionConfigure(): No Ack message from RCE within the timeout.\n" );
    return 4;    
  }
  
  RceFBld::ProxyReplyMsg msgReply;
  memset( &msgReply, 0, sizeof(msgReply) );
  
  iStatus = recvfrom(iSocket, &msgReply, sizeof(msgReply), 0, (struct sockaddr*)&sockaddrServer, (socklen_t*) &iSizeSockAddr);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::onActionConfigure(): recvfrom() failed\n" );
    return 5;      
  }
  
  printf( "Received Reply: Damage %d\n", msgReply.damage.value() );
  *pDamageFromRce = msgReply.damage;
    
  close(iSocket);

  //Client udpClient(0, sizeof(_msg));
  //unsigned int uRceAddr = ntohl( inet_addr( _sRceIp.c_str() ) );  
  //Ins insRce( uRceAddr,  RceFBld::ProxyMsg::ProxyPort );
  //udpClient.send(NULL, (char*) &_msg, sizeof(_msg), insRce);

  //printf(" Sleeping for 500ms to allow RCE time to configure\n");
  //timespec _sleepTime, _fooTime;
  //_sleepTime.tv_sec = 0;
  //_sleepTime.tv_nsec = 500000000;
  //if (nanosleep(&_sleepTime, &_fooTime)<0) perror("nanosleep in RceProxyManager::onActionConfigure");
  return 0;
}

int RceProxyManager::onActionUnmap(const Allocation& alloc)
{
  RceFBld::ProxyMsg msg;
  memset( &msg, 0, sizeof(msg) );

  Client udpClient(0, sizeof(msg));

  unsigned int uRceAddr = ntohl( inet_addr( _sRceIp.c_str() ) );

  Ins insRce( uRceAddr,  RceFBld::ProxyMsg::ProxyPort );
  udpClient.send(NULL, (char*) &msg, sizeof(msg), insRce);

  printf( "Sent %d bytes to RCE %s/%d (Unmap)\n", sizeof(msg), _sRceIp.c_str(),  RceFBld::ProxyMsg::ProxyPort );
  DetInfo& detInfo = (DetInfo&) msg.detInfoSrc;
  printf( "Detector %s Id %d  Device %s Id %d\n", DetInfo::name( detInfo.detector() ), detInfo.detId(),
      DetInfo::name( detInfo.device() ), detInfo.devId() );

  return 0;
}

int RceProxyManager::onActionMap(const Allocation& alloc)
{
  unsigned partition= alloc.partitionid();

  //  setup EVR server
  Ins insEvr(StreamPorts::event(partition, Level::Segment));
  printf( "EVR MC: 0x%x/%d\n", insEvr.address(), insEvr.portId() );

  int iSrcId = -1;
  int iNumSrcLevelNode = 0;
  vector<Ins> vInsEvent;
  for (int iNode=0; iNode < (int) alloc.nnodes(); iNode++)
  {
    const Node& node = *alloc.node(iNode);
    if (node.level()==_selfNode.level())
    {
      if ( node == _selfNode )
      {
        iSrcId = iNumSrcLevelNode;
        break;
      }
      iNumSrcLevelNode++;
    }
  }

  if ( iSrcId == -1 )
  {
    printf("RceProxyManager::onActionMap(): Self node is not found in the allocation list\n" );
    return 1;
  }

  int iDstId = 0;
  for (int iNode=0; iNode < (int) alloc.nnodes(); iNode++)
  {
    const Node& node = *alloc.node(iNode);
    if (node.level()==Level::Event)
    {
      // Add vectored output clients on bld_wire
      Ins insEvent = StreamPorts::event(partition, Level::Event, iDstId, iSrcId );
      vInsEvent.push_back(insEvent);
      printf("Event MC %d: 0x%x/%d\n", iDstId, insEvent.address(), insEvent.portId());
      iDstId++;
    }
  }

  if ( vInsEvent.size() > RceFBld::ProxyMsg::MaxEventLevelServers )
  {
    printf( "RceProxyManager::onActionMap(): Too many (%d) Event Level Servers (> %d)\n", vInsEvent.size(),
        RceFBld::ProxyMsg::MaxEventLevelServers );
    return 2;
  }

  setupProxyMsg( insEvr, vInsEvent, _iNumLinks, _iPayloadSizePerLink, _selfNode.procInfo(), _cfg.src(), _typeidData );

  return 0;
}

int RceProxyManager::setupProxyMsg( const Ins& insEvr, const vector<Ins>& vInsEvent, int iNumLinks, 
    int iPayloadSizePerLink, const ProcInfo& procInfo, const Src& srcProxy, TypeId typeidData)
{
  memset( &_msg, 0, sizeof(_msg) );
  _msg.byteOrderIsBigEndian = 0;
  _msg.numberOfEventLevels = vInsEvent.size();

  for ( int iEvent = 0; iEvent < (int) vInsEvent.size(); iEvent++ )
  {
    _msg.mcAddrs[iEvent].mcaddr = vInsEvent[iEvent].address();
    _msg.mcAddrs[iEvent].mcport = vInsEvent[iEvent].portId();
  }

  _msg.evrMcAddr.mcaddr = insEvr.address();
  _msg.evrMcAddr.mcport = insEvr.portId();

  _msg.payloadSizePerLink = iPayloadSizePerLink;
  _msg.numberOfLinks      = iNumLinks;
  _msg.procInfoSrc        = procInfo;
  _msg.detInfoSrc         = srcProxy;
  _msg.contains           = typeidData;

  return 0;
}
