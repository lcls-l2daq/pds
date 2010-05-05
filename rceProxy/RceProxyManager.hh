#ifndef RCE_PROXY_MANAGER_HH
#define RCE_PROXY_MANAGER_HH

#include <string>
#include <vector>
#include "RceProxyMsg.hh"

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
  class Action;
  class Damage;

  class RceProxyManager
  {
    public:
      RceProxyManager(CfgClientNfs& cfg, const std::string& sRceIp, const std::string& sConfigFile, int iNumLinks, int iPayloadSizePerLink,
          TypeId typeidData, int iTsWidth, int iPhase, const Node& selfNode, int iDebugLevel);
      ~RceProxyManager();

      Appliance& appliance() { return *_pFsm; }

      // event handlers
      int onActionMap(const Allocation& allocation);
      int onActionUnmap(const Allocation& allocation, Damage& damageFromRce);
      int onActionConfigure(Damage& damageFromRce);           

    private:
      int sendMessageToRce(const RceFBld::ProxyMsg& msg, RceFBld::ProxyReplyMsg& msgReply);    

      std::string           _sRceIp;
      std::string           _sConfigFile;
      int                   _iNumLinks;
      int                   _iPayloadSizePerLink;
      TypeId                _typeidData;
      int                   _iTsWidth;
      int                   _iPhase;
      const Node&           _selfNode;
      int                   _iDebugLevel;

      CfgClientNfs&         _cfg;
      Fsm*                  _pFsm;
      //     RceProxyConfigAction* _pActionConfig;
      Action*               _pActionConfig;
      Action*               _pActionMap;
      Action*               _pActionUnmap;
      Action*               _pActionL1Accept;
      Action*               _pActionDisable;

      /* static private member attributes */

      static RceFBld::ProxyMsg    _msg;

      /* static private functions */
      static int setupProxyMsg( const Ins& insEvr, const std::vector<Ins>& vInsEvent, int iNumLinks,
          int iPayloadSizePerLink, const ProcInfo& procInfo, const Src& srcProxy, TypeId typeidData, int iTsWidth, int iPhase );
  };

} // namespace Pds

#endif
