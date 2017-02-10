#ifndef Pds_IbEb_CmpRecvPort_hh
#define Pds_IbEb_CmpRecvPort_hh

#include "pds/ibeb/RdmaPort.hh"

#include "infiniband/verbs.h"

#include <vector>

namespace Pds {
  namespace IbEb {
    class RdmaBase;
    class RdmaComplete;

    class CmpRecvPort : public RdmaPort {
    public:
      CmpRecvPort(int       fd,
                  RdmaBase& base,
                  ibv_mr&   mr,
                  std::vector<char*>& laddr,
                  unsigned  idx);
      ~CmpRecvPort();
    public:
      static void verbose(bool);
    public:
      void complete(unsigned buf);
      RdmaComplete* completion(unsigned buf) const;
    public:
      static void decode(uint32_t, unsigned& src, unsigned& dst);
    private:
      std::vector<ibv_recv_wr> _wr;
      std::vector<char*>       _comps;
    };
  };
};

inline void Pds::IbEb::CmpRecvPort::decode(uint32_t  imm,
                                           unsigned& dst,
                                           unsigned& dstIdx)
{
  dst    = (imm>>0)&0xffff;
  dstIdx = (imm>>16)&0xffff;
}

#endif
