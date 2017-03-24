#ifndef Pds_IbEb_RdmaPort_hh
#define Pds_IbEb_RdmaPort_hh

#include <infiniband/verbs.h>

namespace Pds {
  namespace IbEb {
    class RdmaBase;
    
    class RdmaPort {
    public:
      enum { outlet_port      = 11000,
             completion_port  = 11001 };
    public:
      RdmaPort(int       fd,      // TCP connection
               RdmaBase& base,    // IB device
               ibv_mr&   mr,
               unsigned  idx,     // event builder ID of local end
               unsigned  num_wr,        // max number of outstanding wr allowed
               unsigned  inline_sz=0);  // max size of inline data
      ~RdmaPort();
    public:
      int      fd  () const { return _fd; }  // TCP connection
      ibv_qp*  qp  () const { return _qp; }
      ibv_mr*  mr  () const { return &_mr; }
      unsigned rkey() const { return _rkey; }
      unsigned idx () const { return _idx; } // event builder ID of remote end
    protected:
      int               _fd;
      ibv_qp*           _qp;
      ibv_mr&           _mr;
      unsigned          _rkey;
      unsigned          _idx;
    };
  };
};

#endif
