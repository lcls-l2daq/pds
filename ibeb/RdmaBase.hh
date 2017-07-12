#ifndef Pds_IbEb_RdmaBase_hh
#define Pds_IbEb_RdmaBase_hh

#include <infiniband/verbs.h>
#include <vector>

namespace Pds {
  namespace IbEb {
    class RdmaBase {
    public:
      RdmaBase();
      ~RdmaBase();
    public:
      ibv_context*     ctx() { return _ctx; }
      ibv_pd*          pd () { return _pd; }
      ibv_cq*          cq () { return _cq; }
      ibv_mr*          reg_mr  (void*,size_t);
    protected:
      ibv_context*      _ctx;
      ibv_pd*           _pd;
      ibv_comp_channel* _cc;
      ibv_cq*           _cq;
    };
  };
};

#endif
