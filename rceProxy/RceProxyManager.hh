#ifndef RCE_PROXY_MANAGER_HH
#define RCE_PROXY_MANAGER_HH

#include <string>
#include <vector>
#include "RceProxyMsg.hh"
#include "pdsdata/xtc/DetInfo.hh"

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
      RceProxyManager(CfgClientNfs&, const std::string&, const std::string&,
          TypeId, DetInfo::Device, int, int, const Node&, int);
      ~RceProxyManager();

      Appliance& appliance() { return *_pFsm; }

      // event handlers
      int onActionMap(const Allocation&);
      int onActionUnmap(const Allocation&, Damage&);
      int onActionConfigure(Damage&);
      int sendMessageToRce(const RceFBld::ProxyMsg& msg, RceFBld::ProxyReplyMsg& msgReply);

      TypeId&                 typeId() { return _typeidData;}
      DetInfo::Device        device() { return _device;}
      void*                  config() { return _config;}
      unsigned               configSize() { return _configSize;}
      static void            registerInstance(RceProxyManager* i) {_i=i;}
      static RceProxyManager* instance() { return _i; }
      static RceProxyManager* _i;

    private:
      enum {chunkSize=1<<13};
      int sendConfigToRce(void*, unsigned, RceFBld::ProxyReplyMsg&);

      std::string           _sRceIp;
      std::string           _sConfigFile;
      TypeId                _typeidData;
      DetInfo::Device       _device;
      int                   _iTsWidth;
      int                   _iPhase;
      const Node&           _selfNode;
      int                   _iDebugLevel;


      CfgClientNfs&         _cfg;
      void*                 _config;
      unsigned              _configSize;
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
      static int setupProxyMsg( const Ins& insEvr, const std::vector<Ins>& vInsEvent,
          const ProcInfo& procInfo, const Src& srcProxy, TypeId typeidData, int iTsWidth, int iPhase );
  };

} // namespace Pds

#endif
