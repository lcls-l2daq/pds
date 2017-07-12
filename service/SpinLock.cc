#include "pds/service/SpinLock.hh"

Pds::SpinLock::SpinLock() : _lock(ATOMIC_FLAG_INIT)
{
}

void Pds::SpinLock::lock() 
{
  while( _lock.test_and_set(std::memory_order_acquire) )
    ;
}

void Pds::SpinLock::release() 
{
  _lock.clear(std::memory_order_release);
}
