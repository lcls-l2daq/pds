#ifndef Pds_KStream_hh
#define Pds_KStream_hh

struct iovec;

namespace Pds {

  class ZcpFragment;

  class KStream {
  public:
    KStream ();
    KStream (const KStream&);
    ~KStream();

    int insert(ZcpFragment&, int size);
    int remove(int size);
    int remove(ZcpFragment&, int size);

    int map  (iovec*,int);
    int unmap(iovec*,int);
  private:
    int _fd[2];
  };

}

#endif
