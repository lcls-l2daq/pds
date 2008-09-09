/*
** ++
**  Package:
**	Services
**
**  Abstract:
**      This class wraps the "C" socket (UDP transmitting side) API into
**      something more palatable to our OOP world. The "send" function is
**      called to transmit a UDP datagram. By convention, all sent data is
**      arbitrarily broken into a top (the datagram) and a bottom (the
**      payload) piece. The divisions for these pieces are defined as
**      arguments to both the send function and the class's's constructor.
**      The corresponding UDP reception class is "Server".
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

#ifndef PDS_CLIENT
#define PDS_CLIENT

#include "Port.hh"

namespace Pds {
class Client : public Port
  {
  public:
    Client(const Ins& src,
	      int sizeofDatagram,
	      int maxPayload,
	      const Ins& interface,
	      int maxDatagrams = 1);
    Client(const Ins& src,
              int sizeofDatagram,
              int maxPayload,
              const Ins& dst,
	      const Ins& interface,
	      int maxDatagrams = 1);
    Client(int sizeofDatagram,
              int maxPayload,
              int maxDatagrams = 1);
    Client(int sizeofDatagram,
              int maxPayload,
	      const Ins& interface,
              int maxDatagrams = 1);
    Client(int sizeofDatagram,
              int maxPayload,
              const Ins& dst,
	      const Ins& interface,
              int maxDatagrams = 1);

    ~Client();

  public:
    void use(unsigned interface); 
    int send(char* datagram, struct iovec* msgArray, unsigned msgCount,
             const Ins& dst);
    int send(char* datagram, char* payload, int sizeofPayload);
    int send(char* datagram, char* payload, int sizeofPayload, const Ins&);
    int send(char* datagram,
             char* payload1,
             char* payload2,
             int sizeofPayload1,
             int sizeofPayload2);
    int send(char* datagram,
             char* payload1,
             char* payload2,
             int sizeofPayload1,
             int sizeofPayload2,
             const Ins&);
  private:
     enum {SendFlags = 0};
#ifdef ODF_LITTLE_ENDIAN
    // Would prefer to have _swap_buffer in the stack, but it triggers
    // a g++ bug (tried release 2.96) with pointers to member
    // functions of classes which are bigger than 0x7ffc bytes; this
    // bug appeared in Outlet which points to OutletWire member
    // functions; hence OutletWire must be <= 0x7ffc bytes; if
    // Client is too large the problem appears in OutletWire
    // since it contains an Client. 
    // Later note: g++ 3.0.2 doesn't show this problem.
    char* _swap_buffer;
    void _swap(const iovec* iov, unsigned msgcount, iovec* iov_swap);
#endif
  };
}
#endif
