#ifndef ZcpFragment_hh
#define ZcpFragment_hh

#include "pds/service/LinkedList.hh"

#include "Pool.hh"

struct iovec;

namespace Pds {

  //  Just a pipe
  class ZcpFragment : public LinkedList<ZcpFragment> {
  public:
    ZcpFragment();
    ZcpFragment(const ZcpFragment&);
    ~ZcpFragment();

    void* operator new(unsigned,Pool* p);
    void  operator delete(void*);

    //  Remaining contents
    int size() const { return _size; }

    //  Appending data
    int kinsert(int fd, int size);  // kernel-space
    int uinsert(void* b, int size); // user-space
    int vinsert(iovec* iov, int n); // user-space vector
    int insert(Pds::ZcpFragment&, int size);
    int copy  (Pds::ZcpFragment&, int size);

    //  Extracting data
    int kremove(int size);
    int kremove(int fd, int size);  // kernel-space
    int uremove(void* b, int size); // user-space
    int vremove(iovec* iov, int n); // user-space vector
    int read   (void* b, int size);

    //  Extracting without removing
    int kcopyTo(int fd, int size);

    int moveFrom(ZcpFragment& f, int size) { return insert(f,size); } // move data from another fragment "f"
    int copyFrom(ZcpFragment& f, int size) { return copy  (f,size); } // copy data from another fragment "f"

  private:
    int _fd[2];
    int _size;
  };

}


inline void* Pds::ZcpFragment::operator new(unsigned size,Pds::Pool* pool)
{
  return pool->alloc(size);
}

inline void  Pds::ZcpFragment::operator delete(void* buffer)
{
  Pds::Pool::free(buffer);
}

#endif
