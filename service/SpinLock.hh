#ifndef PDS_SPINLOCK_HH
#define PDS_SPINLOCK_HH

#include <atomic>

namespace Pds {
class SpinLock {
 public:
  SpinLock();
  virtual void lock();
  virtual void release();
private:
  std::atomic_flag _lock;
};
}
#endif



















