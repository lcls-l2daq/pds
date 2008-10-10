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

    //    int insert(char*       , int size);
    //    int insert(TStream&  , int size);
    int insert(ZcpFragment&, int size);
    //    int remove(int size);
    int remove(ZcpFragment&, int size);

    //    int map  (iovec*,int);
    //    int unmap(iovec*,int);
  private:
    int _fd[2];
  };

}

#endif
