#ifndef Pds_IbEb_RdmaRdPort_hh
#define Pds_IbEb_RdmaRdPort_hh

#include "pds/ibeb/RdmaPort.hh"

#include "infiniband/verbs.h"

#include <vector>

namespace Pds {
  namespace IbEb {
    class RdmaBase;
    
    class RdmaRdPort : public RdmaPort {
    public:
      RdmaRdPort(int       fd,
                 RdmaBase& base,
                 ibv_mr&   mr,
                 ibv_mr&   rmr,
                 std::vector<char*>& laddr,
                 unsigned  idx,
                 unsigned  wr_max);
      ~RdmaRdPort();
    public:
      static void verbose(bool);
    public:
      void     req_read(unsigned idx,
                        uint64_t raddr,
                        void*    p,
                        unsigned psize);
      void     complete(unsigned buf);
      char*    push(unsigned idx) const { return _push[idx]; }
      char*    pull(unsigned idx) const { return _pull[idx]; }
      char*    rpull(unsigned idx) const { return _rpull[idx]; }
    public:
      static void decode(uint32_t, unsigned& src, unsigned& dst);
      static void decode(uint64_t, unsigned& src, unsigned& dst);
    private:
      unsigned _encode(unsigned dst);
    private:
      std::vector<ibv_recv_wr> _wr;
      std::vector<ibv_send_wr> _swr;
      std::vector<char*> _push;
      std::vector<char*> _pull;
      std::vector<char*> _rpull;
      unsigned           _dst;
      unsigned           _swrid;
    };
  };
};

inline unsigned Pds::IbEb::RdmaRdPort::_encode(unsigned dst)
{
  return (dst<<16) | idx();
}

inline void Pds::IbEb::RdmaRdPort::decode(uint64_t  wrid,
                                          unsigned& src,
                                          unsigned& dst)
{
  src = (wrid>>32)&0xffff;
  dst = (wrid>>48)&0xffff;
}

inline void Pds::IbEb::RdmaRdPort::decode(uint32_t  imm,
                                          unsigned& src,
                                          unsigned& dst)
{
  src = (imm>>0)&0xffff;
  dst = (imm>>16)&0xffff;
}


#endif
