#ifndef PDS_ABSLOCK_HH
#define PDS_ABSLOCK_HH

namespace Pds {
class AbsLock {
 public:
  virtual ~AbsLock() {}
  virtual void lock()=0;
  virtual void release()=0;
};
}
#endif



















