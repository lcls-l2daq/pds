/*
** ++
**  Package:
**	utility
**
**  Abstract:
**      The purpose of this class is to provide memory allocations without removing them
**      from the pool of available memory.  Thus, the memory is reused, and no cleanup is
**      needed.  
**      
**  Author:
**      Matt Weaver, SLAC, (650) 926-5147
**
**  Creation Date:
**	000 - June 2, 2002
**
**  Revision History:
**	None.
**
** --
*/

#include "ReusePool.hh"

using namespace Pds;

ReusePool::ReusePool(size_t sizeofObject, int numberofObjects) :
  GenericPool(sizeofObject, numberofObjects)
{}

ReusePool::ReusePool(size_t sizeofObject, int numberofObjects, unsigned alignBoundary) :
  GenericPool(sizeofObject, numberofObjects, alignBoundary)
{}

ReusePool::~ReusePool() 
{}

