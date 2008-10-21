/*
** ++
**  Package:
**	odfService
**
**  Abstract:
**
**      Enumeration which set the maximum UDP datagram size used to
**      transfer ODF datagrams between client ("odfOutletWire") and
**      server ("odfEbServer"). Both client and MUST agree on these
**      values. The value MUST be expressed as a power of two. It
**      makes no sense (except as a debugging tool) to set this value
**      less than the hardware MTU size (nominally for ethernet around
**      1500 bytes). Conversely it cannot be set higher than the IP
**      enforced maximum, which is 64 kilobytes, however, which on our
**      architectures (solaris and VxWorks) may be lower.
**
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 1,1998
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_MTU
#define PDS_MTU

namespace Pds {
class Mtu
  {
  public:
//     enum {SizeAsPowerOfTwo = 15};                       // Currently 32 KBytes
    enum {SizeAsPowerOfTwo = 13};                       // Currently 16 KBytes
    enum {Size             = (1 << SizeAsPowerOfTwo)};
    enum {SizeAsMask       = (Size - 1)};
  };
}
#endif
