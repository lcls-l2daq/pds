#ifndef Pds_IbEb_RdmaEvent_hh
#define Pds_IbEb_RdmaEvent_hh

#include "pds/ibeb/RdmaBase.hh"

#include "pds/ibeb/RdmaRdPort.hh"
#include "pds/ibeb/ibcommon.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/RingPool.hh"
#include "pdsdata/psddl/smldata.ddl.h"

#include <list>
#include <string>
#include <vector>
#include <stdint.h>

typedef Pds::SmlData::ProxyV1 SmlD;

namespace Pds {
  class Datagram;
  class Ins;
  namespace IbEb {
    class RdmaEvent : public RdmaBase {
      enum { MAX_PUSH_SZ=512 };
    public:
      RdmaEvent(unsigned          nbuff,
                unsigned          maxEventSize,
                unsigned          index,
                std::vector<std::string>& remote);
      ~RdmaEvent();
    public:
      static void verbose(bool);
    public:
      const Datagram* datagram(unsigned index);
      void            req_read(unsigned index,
                               const SmlD& proxy);
      void            complete(ibv_wc&);
    private:
      bool _alloc(unsigned src, unsigned dst, unsigned stage);
    public:
      uint64_t ncomplete () const { return _ncomplete; }
      uint64_t ncorrupt  () const { return _ncorrupt; }
      uint64_t nbytespush() const { return _nbytespush; }
      uint64_t nbytespull() const { return _nbytespull; }
    private:
      unsigned                 _id;
      unsigned                 _nbuff;
      unsigned                 _nsrc;
      GenericPool*             _pool;
      RingPool*                _rpool;
      std::vector<RdmaRdPort*> _ports;
      ibv_mr*                  _mr;
      ibv_mr*                  _rmr;
      uint64_t*                _buff;
      uint64_t                 _pid;
      uint64_t                 _ncomplete;
      uint64_t                 _ncorrupt;
      uint64_t                 _nbytespush;
      uint64_t                 _nbytespull;
    };
  };
};

#endif
