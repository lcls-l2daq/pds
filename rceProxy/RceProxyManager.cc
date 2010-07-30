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
#include "pds/rceProxy/RceProxyManager.hh"
#include "pds/rceProxy/RceProxyMsg.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/config/CsPadConfigType.hh"

#include "FakeCspadConfig.hh"
#include <time.h>

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
    RceProxyUnmapAction(RceProxyManager& manager) : _manager(manager), _damageFromRce(0) {}

    virtual Transition* fire(Transition* tr)
    {
      const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
      _manager.onActionUnmap(alloc.allocation(), _damageFromRce);
      return tr;
    }
    
    virtual InDatagram* fire(InDatagram* in) 
    {
      if ( _damageFromRce.bits() != 0 ) {
        in->datagram().xtc.damage.increase(_damageFromRce.value());
        in->datagram().xtc.damage.userBits(_damageFromRce.userBits());
      }
      return in;
    }
    
  private:
    RceProxyManager& _manager;
    Damage           _damageFromRce;
};

class RceProxyConfigAction : public Action 
{
  public:
    RceProxyConfigAction(RceProxyManager& manager, CfgClientNfs& cfg, int iDebugLevel) :
      _manager(manager), _cfg(cfg), _iDebugLevel(iDebugLevel), _damageFromRce(0)
      {}

    // this is the first "phase" of a transition where
    // all CPUs configure in parallel.RceProxyConfigAction
    virtual Transition* fire(Transition* tr) 
    {
     // in the long term, we should get this from database - cpo
      //_cfg.fetch(*tr,_epicsArchConfigType, &_config);
      _damageFromRce = Damage(0);
      int iFail = _manager.onActionConfigure(_damageFromRce);
      if ( iFail != 0 ) {
        _damageFromRce.increase(Damage::UserDefined);
      }


      return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
      _cfgtc.src = _cfg.src();
      switch (_manager.device()) {
        case DetInfo::pnCCD :
          _cfgtc.extent = sizeof(Xtc)+sizeof(pnCCDConfigType);
          _cfgtc.contains = TypeId(TypeId::Id_pnCCDconfig, 2);
          break;
        case DetInfo::Cspad :
          _cfgtc.extent = sizeof(Xtc)+sizeof(CsPadConfigType);
          _cfgtc.contains = TypeId(TypeId::Id_CspadConfig, 1);
          break;
        default :
          printf("RceProxyConfigAction.fire(in) Bad device index! %u\n", _manager.device());
      }
      in->insert(_cfgtc, _manager.config());
      
      if ( _damageFromRce.value() != 0 ) {
        in->datagram().xtc.damage.increase(_damageFromRce.bits());
        in->datagram().xtc.damage.userBits(_damageFromRce.userBits());
     }
      return in;
    }

  private:
    RceProxyManager&    _manager;
    CfgClientNfs&       _cfg;
    int                 _iDebugLevel;
    Xtc                 _cfgtc;
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
      //  The RCE should flush all of its L1Accept data before Disable can complete.
      //  Kludge this with a timeout
      timespec ts; ts.tv_nsec = 0; ts.tv_sec = 1;
      nanosleep(&ts,0);
      return in;
    }
  private:
    RceProxyManager& _manager;
};

RceFBld::ProxyMsg RceProxyManager::_msg;

RceProxyManager::RceProxyManager(CfgClientNfs& cfg, const string& sRceIp, const string& sConfigFile,
    TypeId typeidData, DetInfo::Device device, int iTsWidth, int iPhase, const Node& selfNode, int iDebugLevel) :
    _sRceIp(sRceIp), _sConfigFile(sConfigFile),
    _typeidData(typeidData), _device(device), _iTsWidth(iTsWidth),
    _iPhase(iPhase),  _selfNode(selfNode), _iDebugLevel(iDebugLevel), _cfg(cfg), _config(0)
{
  _pFsm            = new Fsm();
  _pActionMap      = new RceProxyAllocAction(*this, cfg);
  _pActionUnmap    = new RceProxyUnmapAction(*this);
  _pActionConfig   = new RceProxyConfigAction(*this, cfg, _iDebugLevel);
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

int RceProxyManager::onActionConfigure(Damage& damageFromRce)
{
  printf("RceProxy Configure transition\n");
  printf( "Sent %d bytes to RCE %s/%d \n", sizeof(_msg), _sRceIp.c_str(), RceFBld::ProxyMsg::ProxyPort);
  DetInfo& detInfo = (DetInfo&) _msg.detInfoSrc;
  printf( "Detector %s Id %d  Device %s Id %d Type id %s ver %d\n", DetInfo::name( detInfo.detector() ), detInfo.detId(),
      DetInfo::name( detInfo.device() ), detInfo.devId(), TypeId::name(_typeidData.id()), _typeidData.version() );
  
  printf( "Traffic shaping width %d  Phase %d\n", _msg.maxTrafficShapingWidth, _msg.trafficShapingInitialPhase );
      
  RceFBld::ProxyReplyMsg msgReply;        
  switch (_device) {
    case DetInfo::pnCCD :
      _configSize = sizeof(pnCCDConfigType);
      _config = calloc(1, _configSize);
      new(_config) pnCCDConfigType(_sConfigFile);
      printf("New pnCCD config from binary file\n");
      break;
    case DetInfo::Cspad :
      _configSize = sizeof(CsPadConfigType);
      _config = calloc(1, _configSize);
      new(_config) pCfg();    // get this from the database !!!!!!  (jackp)
      printf("New CsPad config, quadMask 0x%x\n", (unsigned)((pCfg*)_config)->qm);
      break;
    default :
      printf("RceProxyManager.onActionConfigure() Bad device index! %u\n", _device);
      return 1;
  }
  int iFail = sendMessageToRce( _msg, msgReply );
  if ( iFail != 0 ) return 2;
  if (msgReply.type == RceFBld::ProxyReplyMsg::SendConfig) {
    printf("Sending Configuration to RCE\n");
    int jFail = sendConfigToRce(_config, _configSize, msgReply);
    if ( jFail != 0 ) {
      printf("sendConfigToRce returned %d\n", jFail);
      return 3;
    }
  } else printf("RCE did not ask for configuration\n");
  damageFromRce = msgReply.damage;
  return 0;          
}

int RceProxyManager::onActionUnmap(const Allocation& alloc, Damage& damageFromRce)
{
  RceFBld::ProxyMsg msgUnmap;
  msgUnmap.state = RceFBld::ProxyMsg::UnMapped;
  msgUnmap.byteOrderIsBigEndian = 0;
  
  printf("RceProxy Unmap transition\n");  
  printf( "Sending %d bytes to RCE %s/%d (Unmap)\n", sizeof(msgUnmap), _sRceIp.c_str(),  RceFBld::ProxyMsg::ProxyPort );
   
  RceFBld::ProxyReplyMsg msgReply;  
  int iFail = sendMessageToRce( msgUnmap, msgReply );
  if ( iFail != 0 ) return 1;
  damageFromRce = msgReply.damage;
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

  setupProxyMsg( insEvr, vInsEvent, _selfNode.procInfo(), _cfg.src(), _typeidData, _iTsWidth, _iPhase );

  return 0;
}

int RceProxyManager::sendConfigToRce(void* config, unsigned size, RceFBld::ProxyReplyMsg& msgReply) {
  unsigned numberWholeChunks = size / chunkSize;
  unsigned bytesInLastChunk  = size % chunkSize;
  timespec sleepTime, fooTime;
  sleepTime.tv_sec=0;
  sleepTime.tv_nsec = 2000000;
  uint8_t* pt = (uint8_t*)config;
  int iSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if ( iSocket == -1 )
  {
    printf( "RceProxyManager::sendMessageToRce(): socket() failed\n" );
    return 1;
  }

  sockaddr_in sockaddrServer;
  sockaddrServer.sin_family      = AF_INET;
  sockaddrServer.sin_addr.s_addr = inet_addr(_sRceIp.c_str());
  sockaddrServer.sin_port        = htons(RceFBld::ProxyMsg::ProxyPort);

  int iStatus;
  int iSizeSockAddr = sizeof(sockaddr_in);
  for (unsigned i=0; i<numberWholeChunks; i++) {
    iStatus = sendto( iSocket, (char*) pt, chunkSize, 0, (struct sockaddr*)&sockaddrServer, iSizeSockAddr );
    if ( iStatus == -1 )
    {
      printf( "RceProxyManager::sendConfigToRce(): sendto() failed on chunk %u\n", i );
      return 2;
    }
    pt += chunkSize;
    nanosleep(&sleepTime, &fooTime);
  }
  if (bytesInLastChunk) {
    iStatus = sendto( iSocket, (char*) pt, bytesInLastChunk, 0, (struct sockaddr*)&sockaddrServer, iSizeSockAddr );
    if ( iStatus == -1 ) {
      printf( "RceProxyManager::sendConfigToRce(): sendto() failed on last chunk\n");
      return 3;
    }
  }
  printf( "Sent Config to Rce\n" );
  pt = (uint8_t*)config;
  for (unsigned i=0; i<numberWholeChunks; i++) {
    iStatus = recvfrom(iSocket, (char*) pt, chunkSize, 0, (struct sockaddr*)&sockaddrServer, (socklen_t*) &iSizeSockAddr);
    if ( iStatus == -1 )
    {
      printf( "RceProxyManager::sendConfigToRce(): recvfrom() failed on chunk %u\n", i );
      return 4;
    }
    pt += chunkSize;
  }
  if (bytesInLastChunk) {
    iStatus = recvfrom(iSocket, (char*) pt, bytesInLastChunk, 0, (struct sockaddr*)&sockaddrServer, (socklen_t*) &iSizeSockAddr);
    if ( iStatus == -1 ) {
      printf( "RceProxyManager::sendConfigToRce(): recvfrom() failed on last chunk\n");
      return 5;
    }
  }
  printf( "Got Config back from Rce, waiting for reply now\n" );

  timeval timeout = { 2, 0 }; // timeout in 2 secs

  fd_set  fdsetRead;
  FD_ZERO(&fdsetRead);
  FD_SET(iSocket, &fdsetRead);
  iStatus = select(iSocket+1, &fdsetRead, NULL, NULL, &timeout);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::sendConfigToRce(): select() failed, %s\n", strerror(errno) );
    return 6;
  }
  else if ( iStatus == 0 ) // No socket is ready within the timeout
  {
    close(iSocket);
    printf( "RceProxyManager::sendConfigToRce(): No Ack message from RCE within the timeout.\n" );
    return 7;
  }

  memset( &msgReply, 0, sizeof(msgReply) );

  iStatus = recvfrom(iSocket, &msgReply, sizeof(msgReply), 0, (struct sockaddr*)&sockaddrServer, (socklen_t*) &iSizeSockAddr);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::sendConfigToRce(): recvfrom() failed\n" );
    return 8;
  }

  printf( "Received Reply: Damage 0x%x\n", msgReply.damage.bits() );
  if (msgReply.damage.bits() & (1<<Pds::Damage::UserDefined)) {
    printf( "\tUser Defined Damage 0x%x\n", msgReply.damage.userBits());
  }
  if (msgReply.type == RceFBld::ProxyReplyMsg::Done) {
    printf("Rce Replied that it is done Config\n");
  } else printf("Unknown reply type from Rce!\n");

  close(iSocket);

  return 0;
}

int RceProxyManager::sendMessageToRce(const RceFBld::ProxyMsg& msg, RceFBld::ProxyReplyMsg& msgReply)
{
  int iSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if ( iSocket == -1 ) 
  {
    printf( "RceProxyManager::sendMessageToRce(): socket() failed\n" );
    return 1;
  }
      
  sockaddr_in sockaddrServer;
  sockaddrServer.sin_family      = AF_INET;
  sockaddrServer.sin_addr.s_addr = inet_addr(_sRceIp.c_str());
  sockaddrServer.sin_port        = htons(RceFBld::ProxyMsg::ProxyPort);
  
  int iSizeSockAddr = sizeof(sockaddr_in);    
  int iStatus = sendto( iSocket, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&sockaddrServer, iSizeSockAddr );
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::sendMessageToRce(): sendto() failed\n" );
    return 2;
  }
  
  timeval timeout = { 2, 0 }; // timeout in 2 secs
  
  fd_set  fdsetRead;
  FD_ZERO(&fdsetRead);
  FD_SET(iSocket, &fdsetRead);
  iStatus = select(iSocket+1, &fdsetRead, NULL, NULL, &timeout);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::sendMessageToRce(): select() failed, %s\n", strerror(errno) );
    return 3;
  }
  else if ( iStatus == 0 ) // No socket is ready within the timeout
  {
    close(iSocket);
    printf( "RceProxyManager::sendMessageToRce(): No Ack message from RCE within the timeout.\n" );
    return 4;    
  }
  
  memset( &msgReply, 0, sizeof(msgReply) );
  
  iStatus = recvfrom(iSocket, &msgReply, sizeof(msgReply), 0, (struct sockaddr*)&sockaddrServer, (socklen_t*) &iSizeSockAddr);
  if ( iStatus == -1 )
  {
    printf( "RceProxyManager::sendMessageToRce(): recvfrom() failed\n" );
    return 5;      
  }
  
  printf( "Received Reply: Damage 0x%x\n", msgReply.damage.bits() );
  if (msgReply.damage.bits() & (1<<Pds::Damage::UserDefined)) {
    printf( "\tUser Defined Damage 0x%x\n", msgReply.damage.userBits());
  }
    
  close(iSocket);  
  
  return 0;
}

int RceProxyManager::setupProxyMsg( const Ins& insEvr, const vector<Ins>& vInsEvent,
    const ProcInfo& procInfo, const Src& srcProxy, TypeId typeidData, int iTsWidth, int iPhase)
{
  memset( &_msg, 0, sizeof(_msg) );
  _msg.state = RceFBld::ProxyMsg::Mapped;
  _msg.byteOrderIsBigEndian = 0;
  _msg.numberOfEventLevels = vInsEvent.size();

  for ( int iEvent = 0; iEvent < (int) vInsEvent.size(); iEvent++ )
  {
    _msg.mcAddrs[iEvent].mcaddr = vInsEvent[iEvent].address();
    _msg.mcAddrs[iEvent].mcport = vInsEvent[iEvent].portId();
  }

  _msg.evrMcAddr.mcaddr = insEvr.address();
  _msg.evrMcAddr.mcport = insEvr.portId();

  _msg.procInfoSrc                = procInfo;
  _msg.detInfoSrc                 = srcProxy;
  _msg.contains                   = typeidData;
  _msg.maxTrafficShapingWidth     = iTsWidth;
  _msg.trafficShapingInitialPhase = iPhase;
  
  return 0;
}
