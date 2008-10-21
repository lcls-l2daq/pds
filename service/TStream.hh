#ifndef Pds_TStream_hh
#define Pds_TStream_hh

struct iovec;

namespace Pds {

  class ZcpFragment;

  class TStream {
  public:
    TStream ();
    TStream (const TStream&);
    ~TStream();

    int insert(ZcpFragment&, int size);
    int remove(ZcpFragment&, int size);

    int flush();

    int dump() const;
  private:
    int _fd;
  };

}

#endif
