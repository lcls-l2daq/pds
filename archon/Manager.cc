#include "pds/archon/Manager.hh"
#include "pds/archon/Driver.hh"
#include "pds/archon/Server.hh"

#include "pds/config/ArchonConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"


namespace Pds {
  namespace Archon {

    class AllocAction : public Action {
    public:
      AllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());
        return tr;
      }
    private:
      CfgClientNfs& _cfg;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction(Manager& mgr, Driver& driver, CfgClientNfs& cfg) :
        _mgr(mgr),
        _driver(driver),
        _cfg(cfg),
        _cfgtc(_archonConfigType,cfg.src()),
        _occPool(sizeof(UserMessage),1),
        _error(false) {}
      ~ConfigAction() {}
      InDatagram* fire(InDatagram* dg) {
        // insert assumes we have enough space in the input datagram
        dg->insert(_cfgtc,    &_config);
        if (_error) {
          printf("*** Found configuration errors\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = false;

        int len = _cfg.fetch( *tr, _archonConfigType, &_config, sizeof(_config) );

        if (len <= 0) {

        } else {
          _cfgtc.extent = sizeof(Xtc) + sizeof(ArchonConfigType);
          if(!_driver.configure(_config.config())) {
            _error = true;

            UserMessage* msg = new (&_occPool) UserMessage;
            msg->append("Archon: failed to apply configuration.\n");
            _mgr.appliance().post(msg);
          }
        }
        return tr;
      }
    private:
      Manager&          _mgr;
      Driver&           _driver;
      CfgClientNfs&     _cfg;
      ArchonConfigType  _config;
      Xtc               _cfgtc;
      GenericPool       _occPool;
      bool              _error;
    };
  }
}

using namespace Pds::Archon;

Manager::Manager(Driver& driver, Server& server, CfgClientNfs& cfg) : _fsm(*new Pds::Fsm())
{
  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure, new ConfigAction(*this, driver, cfg));
//  _fsm.callback(Pds::TransitionId::Enable   ,&driver);
//  _fsm.callback(Pds::TransitionId::Disable  ,&driver);
//  _fsm.callback(Pds::TransitionId::L1Accept ,&driver);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

