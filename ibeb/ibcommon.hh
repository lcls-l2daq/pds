#ifndef ibcommon_hh
#define ibcommon_hh

#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <vector>

#include <pthread.h>

//
//  Classes to implement an RDMA WRITE/READ framework
//
namespace Pds {

  namespace IbEb {

    class IbEndPt {
    public:
      uint32_t rkey;
      uint32_t qp_num;
      uint16_t lid;
      uint16_t idx;
      uint8_t  gid[16];
    };

    //  Acknowledge RDMA complete
    class RdmaComplete {
    public:
      uint32_t dst;     // event builder ID
      uint32_t dstIdx;  // event builder buffer #
      char*    dg;      // pull datagram addr
    };
  };
};

#endif
