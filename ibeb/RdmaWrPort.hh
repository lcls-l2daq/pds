#ifndef Pds_IbEb_RdmaWrPort_hh
#define Pds_IbEb_RdmaWrPort_hh

#include "pds/ibeb/RdmaPort.hh"

#include <vector>

namespace Pds {
  namespace IbEb {
    class RdmaBase;
    
    class RdmaWrPort : public RdmaPort {
    public:
      RdmaWrPort(int       fd,
                 RdmaBase& base,
                 ibv_mr&   mr,
                 unsigned  idx);
      ~RdmaWrPort();
    public:
      static void verbose(bool);
    public:
      unsigned nbuff()             const { return _raddr.size(); }
      void*    laddr(unsigned idx) const { return _laddr[idx]; }
      void     write(unsigned src,
                     unsigned idx,
                     void*    p,
                     size_t   psize);
    private:
      unsigned              _wr_id;
      std::vector<uint64_t> _raddr;
      std::vector<void*   > _laddr;
    };
  };
};

#endif
