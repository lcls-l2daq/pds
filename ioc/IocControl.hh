#ifndef Pds_IocControl_hh
#define Pds_IocControl_hh

#include "pds/utility/Appliance.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <list>
#include <string>

namespace Pds {
  class IocHostCallback;
  class IocNode;

  class IocControl : public Appliance {
  public:
    IocControl();
    IocControl(const char* offlinerc,
	       const char* instrument,
	       unsigned    station,
	       unsigned    expt_id);
    ~IocControl();
  public:
    /// Advertise the list of cameras available
    void host_rollcall(IocHostCallback*);

    /// Receive the list of cameras that will be recorded
    void set_partition(const std::list<DetInfo>&);

  public:
    /// Act on a DAQ transition
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram* dg) { return dg; }
  private:
    std::string         _offlinerc;  /// Logbook credentials
    std::string         _instrument; /// Instrument
    unsigned            _station;    /// Instrument station
    unsigned            _expt_id;    /// Experiment number

    std::list<IocNode*> _nodes;      /// Available IOCs
    std::list<IocNode*> _selected_nodes; /// Selected IOCs
  };

};

#endif
