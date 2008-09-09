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
**	August 5, 1999 - Created
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

#ifndef PDS_BIT_LIST_HH
#define PDS_BIT_LIST_HH

#ifdef VXWORKS
#  include <vxWorks.h>

// Define HAS_FFS for architectures that have an FFS-like instruction
#  if CPU_FAMILY==PPC
#    define HAS_FFS
#  endif
#endif

namespace Pds {

class BitList
{
public:
  enum { Done = 32 ^ 0x1f };            // cntlzw returns 32 if no bits set

public:
  BitList  (unsigned long initial) : _remaining(initial) {};
  ~BitList () {};

  unsigned int        get();
  static unsigned int get(unsigned long& bits);
  static unsigned int ffs(unsigned long mask);
  static unsigned int count(unsigned long bits);

private:
  unsigned long _remaining;
};

}
/*
   Description
   -----------
   This routine finds the first bit set in a longword.  It is modeled after
   the cntlzw instruction of the PPC which counts from the MSB (the leftmost
   bit) and returns 32 when no bits are set.  There may be a similar instruction
   defined on the SPARC architecture, but I haven't found it yet.  Thus we loop.

   Returns
   -------
   The bit index of the first bit found set (0 = MSB) or 32 if none were found.
*/

inline unsigned int Pds::BitList::ffs(unsigned long mask)
{
  unsigned int n;
#if defined(CPU_FAMILY) && CPU_FAMILY==PPC
  asm volatile ("cntlzw %0,%1" : "=r"(n) : "r"(mask));
#else
  if (!mask)  return 32;
  n = 0;
  while (!(mask & (1 << 31)))
  {
    mask <<= 1;
    ++n;
  }
#endif
  return n;
}

inline unsigned int Pds::BitList::get()
/*
   Description
   -----------
   This routine finds the first bit set in a longword, clears it and returns
   its bit number.  The instruction used (cntlzw) counts from the MSB (the
   leftmost bit) and returns 32 when no bits are set.

   Returns
   -------
   The bit index of the first bit found set (0 = LSB) or Done if none
   were found.
*/
{
  unsigned long mask = _remaining;
  unsigned int  n    = ffs(mask) ^ 0x1f; // Convert to count from LSB
  _remaining = mask & ~((unsigned long) (1 << n));
  return  n;
}

inline unsigned int Pds::BitList::get(unsigned long& remaining)
/*
   Description
   -----------
   This routine finds the first bit set in a longword, clears it and returns
   its bit number.  The instruction used (cntlzw) counts from the MSB (the
   leftmost bit) and returns 32 when no bits are set.

   Returns
   -------
   The bit index of the first bit found set (0 = LSB) or Done if none
   were found.
*/
{
  unsigned int n = ffs(remaining) ^ 0x1f; // Convert to count from LSB
  remaining &= ~((unsigned long) (1 << n));
  return  n;
}

inline unsigned int Pds::BitList::count(unsigned long bits)
/*
   Description
   -----------
   This routine counts the number of bits set in the supplied longword.

   Returns
   -------
   A count from 0 to 32.
*/
{
  unsigned count = 0;
#ifdef HAS_FFS
  while (get(bits) != Done)  ++count;
#else
  while (bits)
  {
    count += bits & 1;
    bits >>= 1;
  }
#endif
  return count;
}

#endif
