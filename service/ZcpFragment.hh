#ifndef ZcpFragment_hh
#define ZcpFragment_hh

#include "pds/service/LinkedList.hh"

struct iovec;

namespace Pds {

  //  Just a pipe
  class ZcpFragment : public LinkedList<ZcpFragment> {
  public:
    ZcpFragment();
    ~ZcpFragment();

    //  Remaining contents
    int size() const { return _size; }

    //  Appending data
    int kinsert(int fd, int size);  // kernel-space
    int uinsert(const void* b, int size); // user-space
    int vinsert(iovec* iov, int n); // user-space vector
    //    int insert(Pds::ZcpFragment&, int size);
    int copy  (Pds::ZcpFragment&, int size);

    //  Extracting data
    int kremove(int size);
    int kremove(int  fd, int size); // kernel-space
    int uremove(void* b, int size); // user-space
    int vremove(iovec* iov, int n); // user-space vector
    int read   (void* b, int size);

    //  Extracting without removing
    int kcopyTo(int fd, int size);

    void flush();
  private:
    int _fd[2];
    int _size;
  };

}


#endif
