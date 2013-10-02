/*
** ++
**  Package:
**	Container
**
**  Abstract:
**     non-inline functions for "XtcIterator.hh"
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

#include "XtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include <stdio.h>

using namespace Pds;

/*
** ++
**
**   Iterate over the collection specifed as an argument to the function.
**   For each "Xtc" found call back the "process" function. If the
**   "process" function returns zero (0) the iteration is aborted and
**   control is returned to the caller. Otherwise, control is returned
**   when all elements of the collection have been scanned.
**
** --
*/

int PdsClient::XtcIterator::iterate(const Xtc& xtc, InDatagramIterator* root) 
  {
    int sizeofPayload = xtc.sizeofPayload();

    if (xtc.damage.value() & ( 1 << Damage::IncompleteContribution)) {
      root->skip(sizeofPayload);
      return sizeofPayload;
    }

    int remaining = sizeofPayload;
    iovec iov[1];

    while(remaining > 0) {
      int nbytes = root->read(iov,1,sizeof(Xtc));
      if (nbytes != sizeof(Xtc)) {
	printf("XtcIterator::iterate read failed %d/%d/%d bytes\n",
	       nbytes,(unsigned) sizeof(Xtc),remaining);
	return -1;
      }
      const Xtc& xtc = *(Xtc*)iov[0].iov_base;
      remaining -= sizeof(Xtc);

      int xlen = xtc.sizeofPayload();
      if (xlen < 0) {
	printf("xtc payload %d bytes\n", xlen);
	const unsigned* d = reinterpret_cast<const unsigned*>(&xtc);
	printf(":%08x:%08x:%08x:%08x:%08x\n",d[0],d[1],d[2],d[3],d[4]);
	return xlen;
      }
      int len = process(xtc, root);
      if (len < 0) {
	root->skip(remaining);
	return sizeofPayload;
      }
      if (len < xlen) {
	root->skip(xlen-len);
	len = xlen;
      }

      remaining -= len;
    }

    return xtc.sizeofPayload();
  }
