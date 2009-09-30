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

class RceProxyConfigAction : public Action 
{
public:
    RceProxyConfigAction(RceProxyManager& manager, const Src& src, CfgClientNfs& cfg, int iDebugLevel) :
        _manager(manager), _src(src), _cfg(cfg), _iDebugLevel(iDebugLevel)
    {}
    
    // this is the first "phase" of a transition where
    // all CPUs configure in parallel.
    virtual Transition* fire(Transition* tr) 
    {
        //_cfg.fetch(*tr,_epicsArchConfigType, &_config);
        return tr;
    }

    // this is the second "phase" of a transition where everybody
    // records the results of configure (which will typically be
    // archived in the xtc file).
    virtual InDatagram* fire(InDatagram* in) 
    {
        return in;
    }

private:
    //RceProxyConfigType _config;
    //Xtc _cfgtc;
    RceProxyManager&   _manager;
    Src                 _src;
    CfgClientNfs&       _cfg;
    int                 _iDebugLevel;
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

RceProxyManager::RceProxyManager(CfgClientNfs& cfg, const string& sRceIp, const Node& selfNode, int iDebugLevel) :
  _sRceIp(sRceIp), _selfNode(selfNode), _iDebugLevel(iDebugLevel), _cfg(cfg)
{
    _pFsm            = new Fsm();    
    _pActionMap      = new RceProxyAllocAction(*this, cfg);
    _pActionConfig   = new RceProxyConfigAction(*this, RceProxyManager::srcLevel, cfg, _iDebugLevel);
    _pActionL1Accept = new RceProxyL1AcceptAction(*this, _iDebugLevel);
    _pActionDisable  = new RceProxyDisableAction(*this);
    
    _pFsm->callback(TransitionId::Map,        _pActionMap);
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
    
    if ( vInsEvent.size() > RcePnccd::ProxyMsg::MaxEventLevelServers )
    {
        printf( "RceProxyManager::onActionMap(): Too many (%d) Event Level Servers (> %d)\n", vInsEvent.size(), 
          RcePnccd::ProxyMsg::MaxEventLevelServers );
        return 2;
    }

    RcePnccd::ProxyMsg msg;
    setupProxyMsg( insEvr, vInsEvent, _cfg.src(), msg );
    
    Client udpClient(0, sizeof(msg)); 
    
    unsigned int uRceAddr = ntohl( inet_addr( _sRceIp.c_str() ) );
        
    Ins insRce( uRceAddr,  RcePnccd::ProxyMsg::ProxyPort );
    udpClient.send(NULL, (char*) &msg, sizeof(msg), insRce);
    
    printf( "Sent %d bytes to RCE %s/%d\n", sizeof(msg), _sRceIp.c_str(),  RcePnccd::ProxyMsg::ProxyPort );
    DetInfo& detInfo = (DetInfo&) msg.src;
    printf( "Detector %s Id %d  Device %s Id %d\n", DetInfo::name( detInfo.detector() ), detInfo.detId(),
      DetInfo::name( detInfo.device() ), detInfo.devId() );
    
    return 0;
}

int RceProxyManager::setupProxyMsg( const Ins& insEvr, const vector<Ins>& vInsEvent, const Src& srcProxy, RcePnccd::ProxyMsg& msg )
{
    msg.byteOrderIsBigEndian = 0;
    msg.numberOfEventLevels = vInsEvent.size();
    
    for ( int iEvent = 0; iEvent < (int) vInsEvent.size(); iEvent++ )
    {
        msg.mcAddrs[iEvent].mcaddr = vInsEvent[iEvent].address();
        msg.mcAddrs[iEvent].mcport = vInsEvent[iEvent].portId();
    }
    
    msg.evrMcAddr.mcaddr = insEvr.address();
    msg.evrMcAddr.mcport = insEvr.portId();

    msg.payloadSizePerLink = 0;
    msg.numberOfLinks = 2;
    msg.src = srcProxy;
    
    return 0;
}
