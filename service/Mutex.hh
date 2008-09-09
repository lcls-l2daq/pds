#ifndef PDS_MUTEX_HH
#define PDS_MUTEX_HH


#ifdef VXWORKS
#  include "semLib.h"
#else //solaris
#endif

// cpo adopted this class from Semaphore to obtain a semaphore that
// can be taken recursively by the same task (in vxworks).
namespace Pds {
class Mutex {
 public:
  Mutex();
  ~Mutex();
  void take();
  void give();

 private:

#ifdef VXWORKS
  SEM_ID _sem;
#endif

};
}
#endif
