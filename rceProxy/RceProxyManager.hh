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
//  using namespace RcePnccd;

  class Allocation;
  class Src;
  class Ins;
  class Fsm;
  class Appliance;
  class CfgClientNfs;
  class Action;

  class RceProxyManager
  {
    public:
      RceProxyManager(CfgClientNfs& cfg, const std::string& sRceIp, int iNumLinks, int iPayloadSizePerLink,
          const Node& selfNode, int iDebugLevel);
      ~RceProxyManager();

      Appliance& appliance() { return *_pFsm; }

      // event handlers
      int onActionMap(const Allocation& allocation);
      int onActionUnmap(const Allocation& allocation);
      int onActionConfigure();

    private:
      static const Src srcLevel; // Src for Epics Archiver

      std::string           _sRceIp;
      int                   _iNumLinks;
      int                   _iPayloadSizePerLink;
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

      static RcePnccd::ProxyMsg    _msg;

      /* static private functions */
      static int setupProxyMsg( const Ins& insEvr, const std::vector<Ins>& vInsEvent, int iNumLinks,
          int iPayloadSizePerLink, const ProcInfo& procInfo, const Src& srcProxy );
  };

} // namespace Pds

#endif
