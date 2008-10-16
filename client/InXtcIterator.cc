/*
** ++
**  Package:
**	Container
**
**  Abstract:
**     non-inline functions for "InXtcIterator.hh"
**      
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - October 11,1998
**
**  Revision History:
**	None.
**
** --
*/

#include "InXtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include <stdio.h>

using namespace Pds;

/*
** ++
**
**   Iterate over the collection specifed as an argument to the function.
**   For each "InXtc" found call back the "process" function. If the
**   "process" function returns zero (0) the iteration is aborted and
**   control is returned to the caller. Otherwise, control is returned
**   when all elements of the collection have been scanned.
**
** --
*/

int InXtcIterator::iterate(const InXtc& xtc, InDatagramIterator* root) 
  {
    //    if (xtc.damage.value() & ( 1 << Damage::IncompleteContribution))
    //      return -1;

    int remaining = xtc.sizeofPayload();
    iovec iov[1];

    while(remaining > 0) {
      int nbytes = root->read(iov,1,sizeof(InXtc));
      if (nbytes != sizeof(InXtc)) {
	printf("InXtcIterator::iterate read failed %d/%d bytes\n",nbytes,sizeof(InXtc));
	return -1;
      }
      const InXtc& inXtc = *(InXtc*)iov[0].iov_base;
      remaining -= sizeof(InXtc);

      int xlen = inXtc.sizeofPayload();
      if (xlen < 0) {
	printf("inXtc payload %d bytes\n", xlen);
	return xlen;
      }
      int len = process(inXtc, root);
      if (len < 0) return len;
      if (len < xlen)
	root->skip(xlen-len);

      remaining -= xlen;
    }

    return xtc.sizeofPayload();
  }
