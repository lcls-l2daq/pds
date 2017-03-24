#ifndef Pds_IbEb_CmpSendPort_hh
#define Pds_IbEb_CmpSendPortPort_hh

#include "pds/ibeb/RdmaPort.hh"

#include <vector>

namespace Pds {
  namespace IbEb {
    class RdmaBase;

    class CmpSendPort : public RdmaPort {
    public:
      CmpSendPort(int       fd,
                  RdmaBase& base,
                  ibv_mr&   mr,
                  std::vector<void*> laddr,
                  unsigned  idx,
                  unsigned  wr_max);
      ~CmpSendPort();
    public:
      static void verbose(bool);
    public:
      unsigned nbuff()             const { return _raddr.size(); }
      void*    laddr(unsigned idx) const { return _laddr[idx]; }
      void     ack(unsigned buf, char* dg);
    public:
      static unsigned encode(unsigned dst, unsigned dstIdx);
    private:
      unsigned              _wr_id;
      unsigned              _src;
      std::vector<uint64_t> _raddr;
      std::vector<void*   > _laddr;
    };
  };
};

inline unsigned Pds::IbEb::CmpSendPort::encode(unsigned dst, unsigned dstIdx)
{
  return (dstIdx<<16) | dst;
}

#endif
