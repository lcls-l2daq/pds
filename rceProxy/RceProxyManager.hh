#ifndef RCE_PROXY_MANAGER_HH
#define RCE_PROXY_MANAGER_HH

#include <string>
#include <vector>

namespace RcePnccd
{
    class ProxyMsg;
}

namespace Pds 
{

class Allocation;
class Src;
class Ins;
class Fsm;
class Appliance;
class CfgClientNfs;

class RceProxyManager 
{
public:
    RceProxyManager(CfgClientNfs& cfg, const std::string& sRceIp, int iNumLinks, int iPayloadSizePerLink, 
      const Node& selfNode, int iDebugLevel);
    ~RceProxyManager();

    Appliance& appliance() { return *_pFsm; }

    // event handlers
    int onActionMap(const Allocation& allocation);
    
private:  
    static const Src srcLevel; // Src for Epics Archiver
    
    std::string         _sRceIp;
    int                 _iNumLinks;
    int                 _iPayloadSizePerLink;    
    const Node&         _selfNode;
    int                 _iDebugLevel;    

    CfgClientNfs&       _cfg;
    Fsm*                _pFsm;
    Action*             _pActionConfig;
    Action*             _pActionMap;
    Action*             _pActionL1Accept;
    Action*             _pActionDisable;
    
    /* static private functions */
    static int setupProxyMsg( const Ins& insEvr, const std::vector<Ins>& vInsEvent, int iNumLinks, 
      int iPayloadSizePerLink, const Src& srcProxy, RcePnccd::ProxyMsg& msg );
};

} // namespace Pds

#endif
