#ifndef Pds_PayloadIterator_hh
#define Pds_PayloadIterator_hh

#include "InDatagramIterator.hh"

namespace Pds {

  class PayloadIterator : public InDatagramIterator {
  public:
    PayloadIterator( const char*, int);
    ~PayloadIterator() {}

    int   skip(int len);
    int   read(iovec* iov, int maxiov, int len);
    void* read_contiguous(int len, void* buffer);
  private:
    const char*     _base;
    int             _offset;
    int             _end;
 };

}

inline Pds::PayloadIterator::PayloadIterator(const char* base, int len) :
  _base  (base),
  _offset(0),
  _end   (len)
{
}

#endif
