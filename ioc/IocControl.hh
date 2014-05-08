#ifndef Pds_IocControl_hh
#define Pds_IocControl_hh

#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <list>
#include <string>

namespace Pds {
  class IocHostCallback;
  class IocNode;
  class IocConnection;

  class IocControl : public Appliance {
    friend class IocConnection;
  public:
    IocControl();
    IocControl(const char* offlinerc,
	       const char* instrument,
	       unsigned    station,
	       unsigned    expt_id,
               const char* controlrc);
    ~IocControl();
  public:
    /// Write the global configuration to a new connection.
    void write_config(IocConnection *c, unsigned run, unsigned stream);

    /// Advertise the list of cameras available
    void host_rollcall(IocHostCallback*);

    /// Receive the list of cameras that will be recorded
    void set_partition(const std::list<DetInfo>&);

    const std::list<IocNode*>& selected() const { return _selected_nodes; }

  public:
    /// Act on a DAQ transition
    Transition* transitions(Transition*);
    InDatagram* events     (InDatagram* dg) { return dg; }

      //  private:
    void _report_error(const std::string&);

  private:
    std::list<std::string> _offlinerc;   /// Logbook credentials
    std::string         _instrument;     /// Instrument
    unsigned            _station;        /// Instrument station
    unsigned            _expt_id;        /// Experiment number
    int                 _recording;      /// Are we recording now?

    std::list<IocNode*> _nodes;          /// Available IOCs
    std::list<IocNode*> _selected_nodes; /// Selected IOCs

    char _trans[1024];

    GenericPool         _pool;
  };
};

#endif
