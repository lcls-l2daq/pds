/*
** ++
**
**  Facility:
**	Service
**
**  Abstract:
**      Simple class to take care of common alignment problems.
**
**  Author:
**      R. Claus, SLAC/PEP-II BaBar Online Dataflow Group
**
**  History:
**	march 17, 2000 - Created
**
**  Copyright:
**                                Copyright 2000
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

namespace Pds {

class Align
{
public:
  Align() {};
  ~Align() {};

  static void* align4(void* ptr);
  static void* align8(void* ptr);
  static void* align32(void* ptr);

  //static void* align(void* ptr, unsigned n); // left out because too much work
                                        // to verify that n is a power of two
};
}

// 4 byte align
inline void* Pds::Align::align4(void* ptr)
{
  return ((unsigned) ptr & 0x03) ? (void*)(((unsigned) ptr +  4) & ~0x03) : ptr;
}

// 8 byte align
inline void* Pds::Align::align8(void* ptr)
{
  return ((unsigned) ptr & 0x07) ? (void*)(((unsigned) ptr +  8) & ~0x07) : ptr;
}

// 32 byte align
inline void* Pds::Align::align32(void* ptr)
{
  return ((unsigned) ptr & 0x1f) ? (void*)(((unsigned) ptr + 32) & ~0x1f) : ptr;
}
