#ifndef Pds_LeCroy_Server_hh
#define Pds_LeCroy_Server_hh

#include "pds/service/Semaphore.hh"
#include "pds/utility/EbServer.hh"
#include "pds/utility/BldSequenceSrv.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/epicstools/PVSubWaveform.hh"
#include "pds/client/Action.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <vector>

namespace Pds {
  namespace LeCroy {
    class ConfigServer;
    class Server : public EbServer,
                   public EbCountSrv,
                   public Action, 
                   public Pds_Epics::PVMonitorCb {
    public:
      Server(const char*, const DetInfo&, const unsigned);
      virtual ~Server();
    public:
      //  Eb interface
      void       dump ( int detail ) const {}
      bool       isValued( void ) const    { return true; }
      const Src& client( void ) const      { return _xtc.src; }

      //  EbSegment interface
      const Xtc& xtc( void ) const    { return _xtc; }
      unsigned   offset( void ) const { return sizeof(Xtc); }
      unsigned   length() const       { return _xtc.extent; }

      //  Eb-key interface
      EbServerDeclare;

      //  Server interface
      int pend( int flag = 0 ) { return -1; }
      int fetch( char* payload, int flags );

      unsigned count() const;

      Transition* fire(Transition*);
      InDatagram* fire(InDatagram*);

      // PvMonitorCb
      void        updated();
      void        signal(bool);
      enum { NCHANNELS=4 };

    protected:
      // LeCroy specific Server interface
      int  fill( char*, const void* );
      void post(const void*);

      // Update scope readout status
      void fetch_readout();

    private:
      Xtc            _xtc;
      const unsigned _max_length;
      unsigned       _count;
      bool           _readout;
      bool           _ready;
      int            _pfd[2];
      std::vector<Pds_Epics::PVSubWaveform*>  _raw;
      std::vector<ConfigServer*>              _config_pvs;
      std::vector<ConfigServer*>              _offset_pvs;
      std::vector<ConfigServer*>              _period_pvs;
      ConfigServer* _status;
      bool          _enabled;
      unsigned      _chan_mask;
      unsigned      _config_size;
      uint32_t      _data_size;
      uint32_t      _Length[NCHANNELS];
      uint32_t      _SampleType[NCHANNELS];
      int32_t       _Offset[NCHANNELS];
      double        _RawOffset[NCHANNELS];
      double        _Period[NCHANNELS];
      char *        _ConfigBuff;
      char *        _DataBuff;
      Semaphore     _sem;
    };
  };
};

#endif
