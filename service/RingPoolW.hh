/*
** ++
**  Package:
**	Service
**
**  Abstract:
**     Definition of RingPoolW.  This is the resource-wait mode version of
**     RingPool.
**
**  Author:
**      Ric Claus, SLAC
**
**  Creation Date:
**	000 - January 30, 2001
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_RINGPOOLW_HH
#define PDS_RINGPOOLW_HH

#include "RingPool.hh"
#include "Semaphore.hh"

namespace Pds {
class RingPoolW : public RingPool
{
public:
  RingPoolW(size_t size, size_t wrap);
  ~RingPoolW();
public:
  void*         alloc(size_t  size);
  void*         alloc(size_t* size);
public:
  unsigned      stalls()  const;
  unsigned      resumes() const;
  void          dump(int level);
private:
  static void   _free(void* entry, void* buffer);
private:
  unsigned          _stalls;                // Number of times allocation stalled
  unsigned          _resumes;               // Number of times allocation resumed
  volatile unsigned _stalled;               // 1 if stalled, 0 if not
  Semaphore      _resource;              // Semaphore allocation stalls behind
};
}

/*
** ++
**
**    Return the number of times allocation attempts went into
**    resource wait mode.
**
** --
*/

inline unsigned Pds::RingPoolW::stalls() const
{
  return _stalls;
}

/*
** ++
**
**    Return the number of times allocation attempts were released from
**    resource wait mode.
**
** --
*/

inline unsigned Pds::RingPoolW::resumes() const
{
  return _resumes;
}

#endif
