#ifndef Pds_ZcpNullStream_hh
#define Pds_ZcpNullStream_hh

struct iovec;

namespace Pds {

  class ZcpFragment;

  class ZcpNullStream {
  public:
    ZcpNullStream () {}
    ZcpNullStream (const ZcpNullStream&) {}
    ~ZcpNullStream() {}

    int insert(ZcpFragment&, int size) { return 0; }
    int remove(ZcpFragment&, int size) { return 0; }

    int flush() { return 0; }

    int dump() const { return 0; }
  };

}

#endif
