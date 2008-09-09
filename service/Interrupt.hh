/*
** ++
**
**  Facility:
**	Service
**
**  Abstract:
**
**  Author:
**      R. Claus, SLAC/PEP-II BaBar Online Dataflow Group
**
**  History:
**	December 10, 1999 - Created
**
**  Copyright:
**                                Copyright 1999
**                                      by
**                         The Board of Trustees of the
**                       Leland Stanford Junior University.
**                              All rights reserved.
**
**
**         Work supported by the U.S. Department of Energy under contract
**       DE-AC03-76SF00515.
**
**                               Disclaimer Notice
**
**        The items furnished herewith were developed under the sponsorship
**   of the U.S. Government.  Neither the U.S., nor the U.S. D.O.E., nor the
**   Leland Stanford Junior University, nor their employees, makes any war-
**   ranty, express or implied, or assumes any liability or responsibility
**   for accuracy, completeness or usefulness of any information, apparatus,
**   product or process disclosed, or represents that its use will not in-
**   fringe privately-owned rights.  Mention of any product, its manufactur-
**   er, or suppliers shall not, nor is it intended to, imply approval, dis-
**   approval, or fitness for any particular use.  The U.S. and the Univer-
**   sity at all times retain the right to use and disseminate the furnished
**   items for any purpose whatsoever.                       Notice 91 02 01
**
** --
*/

#ifndef PDS_INTERRUPT_HH
#define PDS_INTERRUPT_HH

#include <intLib.h>

namespace Pds {
class Interrupt
{
public:
  static unsigned Interrupt::off();
  static void     Interrupt::on(unsigned key);
};
}

inline unsigned Pds::Interrupt::off()
/*
   Description
   -----------
   Inlineable version of intLock() from VxWorks.  Since this is just
   disassembled from the VxWorks version, it can be used with intUnlock().

   Returns
   -------
   A lock-out key for Pds::Interrupt::on() or intUnlock()
*/
{
#if 1
  // JJ expressed concern that inlining the instructions from intLock allows
  // the compiler to reorder them along with those of the "caller", possibly
  // causing some of the "caller's" instruction not to be protected by the
  // action of the intLock.  Thus RiC put the call to intLock back 4/9/01.

  unsigned state;
  unsigned key;

  asm volatile ("mfmsr  %0"              : "=r"(state));
  asm volatile ("rlwinm %0,%1,0x0,17,15" : "=r"(key)   : "r"(state));
  asm volatile ("mtmsr  %0"              :             : "r"(key));
  asm volatile ("isync");
  //  return state;
  return key;

#else
  if (!intContext()) 
    return intLock();
  else
    return 0;
#endif
}


inline void Pds::Interrupt::on(unsigned key)
/*
   Description
   -----------
   Inlineable version of intUnlock() from VxWorks.  Since this is just
   disassembled from the VxWorks version, it can be used with intLock().

   Argument
   --------
        key - A lock-out key from Pds::Interrupt::off() or intLock()
*/
{
#if 1
  // JJ expressed concern that inlining the instructions from intUnlock allows
  // the compiler to reorder them along with those of the "caller", possibly
  // causing some of the "caller's" instruction not to be protected by the
  // action of the intUnLock.  Thus RiC put the call to intUnlock back 4/9/01.

  unsigned state;

  asm volatile ("rlwinm %0,%1,0x0,16,16" : "=r"(key)   : "0"(key));
  asm volatile ("mfmsr  %0"              : "=r"(state));
  asm volatile ("or     %0,%1,%2"        : "=r"(key)   : "r"(state), "0"(key));
  asm volatile ("mtmsr  %0"              :             : "r"(key));
  asm volatile ("isync");

#else
  if (!intContext()) 
    intUnlock(key);
#endif
}

#endif

