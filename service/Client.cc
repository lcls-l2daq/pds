/*
** ++
**  Package:
**	Services
**
**  Abstract:
**     Non-inline functions for "Client.hh"
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

#include "Client.hh"
#include "Sockaddr.hh"
#include "ZcpFragment.hh"
#include <errno.h>
#include <string.h>

#ifdef VXWORKS
#define msg_control     msg_accrights 
#define msg_controllen  msg_accrightslen
#else
#include <sys/uio.h>
#endif

using namespace Pds;

/*
** ++
**
**    The constructor for the class accepts as arguments, the size (in
**    bytes) of the header portion of any transmitted datagram
**    ("sizeofDatagram"), the maximum size (in bytes) of the trailer
**    portion of any sent datagram ("maxPayload"), and the maximum
**    number of datagrams (defaulted to one) which could be queued at
**    any one time ("maxDatagrams"). Together, these three arguments
**    are used to derive the amount of network buffering used
**    internally by the client's port. The constructor also takes as
**    an argument the interface which should be used to send multicast
**    messages ("interface"). Finally, an argument is passed to
**    specify the network address to to be assigned to the client
**    ("src).
**
** --
*/

Client::Client(const Ins& src,
                     int sizeofDatagram,
                     int maxPayload,
		     const Ins& interface,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          src,
          sizeofDatagram,
          maxPayload,
	  maxDatagrams)
  {
    in_addr address;
    address.s_addr = htonl(interface.address());
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
		   sizeof(in_addr)) < 0) error(errno);
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
  }

/*
** ++
**
**    The constructor for the class accepts as arguments, the size (in
**    bytes) of the header portion of any transmitted datagram
**    ("sizeofDatagram"), the maximum size (in bytes) of the trailer
**    portion of any sent datagram ("maxPayload") and the maximum
**    number of datagrams (defaulted to one) which could be queued at
**    any one time ("maxDatagrams"). Together, these three arguments
**    are used to derive the amount of network buffering used
**    internally by the client's port. The constructor also takes as
**    an argument the interface which should be used to send multicast
**    messages ("interface"). If the destination address is fixed, the
**    caller may specify it address at construction time
**    ("dst). Finally, an argument is passed to specify the network
**    address to to be assigned to the client ("src).
**
** --
*/

Client::Client(const Ins& src,
                     int sizeofDatagram,
                     int maxPayload,
                     const Ins& dst,
		     const Ins& interface,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          src,
          sizeofDatagram,
          maxPayload,
	  maxDatagrams)
  {
    in_addr address;
    address.s_addr = htonl(interface.address());
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
		   sizeof(in_addr)) < 0) error(errno);
    Sockaddr sa(dst);
    if(connect(_socket, sa.name(), sa.sizeofName()) == -1) 
      error(errno);
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
  }

/*
** ++
**
**    The constructor for the class accepts as arguments, the size (in
**    bytes) of the header portion of any transmitted datagram
**    ("sizeofDatagram"), the maximum size (in bytes) of the trailer
**    portion of any sent datagram ("maxPayload") and the maximum
**    number of datagrams (defaulted to one) which could be queued at
**    any one time ("maxDatagrams"). Together, these three arguments
**    are used to derive the amount of network buffering used
**    internally by the client's port.
**
** --
*/

Client::Client(int sizeofDatagram,
                     int maxPayload,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          sizeofDatagram,
          maxPayload,
          maxDatagrams)
  {
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
  }

/*
** ++
**
**    The constructor for the class accepts as arguments, the size (in
**    bytes) of the header portion of any transmitted datagram
**    ("sizeofDatagram"), the maximum size (in bytes) of the trailer
**    portion of any sent datagram ("maxPayload") and the maximum
**    number of datagrams (defaulted to one) which could be queued at
**    any one time ("maxDatagrams"). Together, these three arguments
**    are used to derive the amount of network buffering used
**    internally by the client's port. The constructor also takes as
**    an argument the interface which should be used to send multicast
**    messages ("interface"). In this version a default address is
**    assigned to the client.
**
** --
*/

Client::Client(int sizeofDatagram,
                     int maxPayload,
		     const Ins& interface,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          sizeofDatagram,
          maxPayload,
          maxDatagrams)
  {
    in_addr address;
    address.s_addr = htonl(interface.address());
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
		   sizeof(in_addr)) < 0) error(errno);
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
  }

/*
** ++
**
**    The constructor for the class accepts as arguments, the size (in
**    bytes) of the header portion of any transmitted datagram
**    ("sizeofDatagram"), the maximum size (in bytes) of the trailer
**    portion of any sent datagram ("maxPayload") and the maximum
**    number of datagrams (defaulted to one) which could be queued at
**    any one time ("maxDatagrams"). Together, these three arguments
**    are used to derive the amount of network buffering used
**    internally by the client's port. The constructor also takes as
**    an argument the interface which should be used to send multicast
**    messages ("interface"). If the destination address is fixed, the
**    caller may specify it address at construction time ("dst). In
**    this version of the constructor a default address is assigned to
**    the port.
**
** --
*/


Client::Client(int sizeofDatagram,
                     int maxPayload,
                     const Ins& dst,
		     const Ins& interface,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          sizeofDatagram,
          maxPayload,
          maxDatagrams)
  {
    in_addr address;
    address.s_addr = htonl(interface.address());
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
		   sizeof(in_addr)) < 0) error(errno);
    Sockaddr sa(dst);
    if(connect(_socket, sa.name(), sa.sizeofName()) == -1) 
      error(errno);
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
  }


/*
** ++
**
**    Delete swap buffer under little endian machines
**
** --
*/


Client::~Client() {
#ifdef ODF_LITTLE_ENDIAN
  delete _swap_buffer;
#endif
}

/*
** ++
**
**    Use specified interface for sending multicast addresses
**
** --
*/

void Client::use(unsigned interface)
{
  in_addr address;
  address.s_addr = htonl(interface);
  if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
                 sizeof(in_addr)) < 0) 
    error(errno);
}

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified as the "dst" argument in the class's's constructor.
**    The datagram is specified in two parts: "datagram", which is a
**    pointer the datagram's fixed size header, and "payload", which
**    is a pointer to the datagrams' variable sized portion. The size
**    of the payload (in bytes) is specified by the "sizeofPayload"
**    argument. The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/

int Client::send(char* datagram, char* payload, int sizeofPayload)
  {
    struct msghdr hdr;
    struct iovec  iov[2];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload);
    iov[1].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 2;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload);
    iov[0].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 1;
    }

  hdr.msg_name         = (caddr_t)0;
  hdr.msg_namelen      = 0;
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;

#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
  }

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified by the "dst" argument.
**    The datagram is specified in two parts: "datagram", which is a
**    pointer the datagram's fixed size header, and "payload", which
**    is a pointer to the datagrams' variable sized portion. The size
**    of the payload (in bytes) is specified by the "sizeofPayload"
**    argument. The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/


int Client::send(char*         datagram,
                    char*         payload,
                    int           sizeofPayload,
                    const Ins& dst)
  {
    struct msghdr hdr;
    struct iovec  iov[2];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload);
    iov[1].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 2;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload);
    iov[0].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 1;
    }

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;


#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
  }

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified by the "dst" argument.The datagram is specified
**    in three parts: "datagram", which is a  pointer the datagram's
**    fixed size header, and "payload1" and "payload2", which are pointers
**    to the datagram's variable sized portion. The size of the payload
**    (in bytes) is specified by the "sizeofPayload1" and "sizeofPayload2"
**    arguments. The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/

int Client::send(char*         datagram,
                    char*         payload1,
		    char*         payload2,
                    int           sizeofPayload1,
		    int           sizeofPayload2,
                    const Ins& dst)
  {
    struct msghdr hdr;
    struct iovec  iov[3];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload1);
    iov[1].iov_len  = sizeofPayload1;
    iov[2].iov_base = (caddr_t)(payload2);
    iov[2].iov_len  = sizeofPayload2;
    hdr.msg_iovlen  = 3;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload1);
    iov[0].iov_len  = sizeofPayload1;
    iov[1].iov_base = (caddr_t)(payload2);
    iov[1].iov_len  = sizeofPayload2;
    hdr.msg_iovlen  = 2;
    }

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;


#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
  }

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified as the "dst" argument in the class's's constructor.
**    The datagram is specified in three parts: "datagram", which is a
**    pointer the datagram's fixed size header, and "payload1" and "payload2",
**    which are pointers to the datagram's variable sized portion. The size
**    of the payload (in bytes) is specified by the "sizeofPayload1" and
**    "sizeofPayload2" arguments. The function returns the transmission
**    status. A value of zero (0) indicates the transfer was sucessfull.
**    A positive non-zero value is the reason (as an "errno"value) the
**    transfer failed.
**
** --
*/

int Client::send(char*         datagram,
                    char*         payload1,
		    char*         payload2,
                    int           sizeofPayload1,
		    int           sizeofPayload2)
  {
    struct msghdr hdr;
    struct iovec  iov[3];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload1);
    iov[1].iov_len  = sizeofPayload1;
    iov[2].iov_base = (caddr_t)(payload2);
    iov[2].iov_len  = sizeofPayload2;
    hdr.msg_iovlen  = 3;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload1);
    iov[0].iov_len  = sizeofPayload1;
    iov[1].iov_base = (caddr_t)(payload2);
    iov[1].iov_len  = sizeofPayload2;
    hdr.msg_iovlen  = 2;
    }

  hdr.msg_name         = (caddr_t)0;
  hdr.msg_namelen      = 0;
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;

#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
  }

/*
** ++
**
**
**
** --
*/

int Client::send(char*         datagram,
                    struct iovec* msgArray,
                    unsigned      msgCount,
                    const Ins& dst)
{
  if(datagram)                          // Overwrite the first entry(!)
    {
      msgArray[0].iov_base = (caddr_t)(datagram);
      msgArray[0].iov_len  = sizeofDatagram();
    }
  else                                  // Skip the first entry
    {
      ++msgArray;
      --msgCount;
    }
  
  struct msghdr hdr;
  
  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;


#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(msgArray, msgCount, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov          = msgArray;
  hdr.msg_iovlen       = msgCount;
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
}

/*
** ++
**
**
** --
*/
#include <stdio.h>

int Client::send(char*        datagram,
		 ZcpFragment& payload,
		 int          size,
		 const Ins&   dst)
{
  struct msghdr hdr;
  struct iovec  iov[1];

  iov[0].iov_base = (caddr_t)(datagram);
  iov[0].iov_len  = sizeofDatagram();

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;
  hdr.msg_iov          = iov;
  hdr.msg_iovlen       = 1;

  int length = ::sendmsg(_socket, &hdr, MSG_MORE);
  if (length == -1) {
    printf("Client::sendmsg error\n");
    return errno;
  }

  length = payload.kremove(_socket, size); 
  if (length==-1) {
    printf("Client::kremove error\n");
    return errno;
  }

  if (length != size) {
    printf("Client::sendz1 %d/%d\n",length,size);
    return ENOMEM;
  }

  return 0;
}

int Client::send(char*        datagram,
		 char*        payload1,
		 int          size1,
		 ZcpFragment& payload2,
		 int          size2,
		 const Ins&   dst)
{
  struct msghdr hdr;
  struct iovec  iov[2];

  iov[0].iov_base = (caddr_t)(datagram);
  iov[0].iov_len  = sizeofDatagram();
  iov[1].iov_base = (caddr_t)(payload1);
  iov[1].iov_len  = size1;

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;
  hdr.msg_iov          = iov;
  hdr.msg_iovlen       = 2;

  int length = ::sendmsg(_socket, &hdr, MSG_MORE);
  if (length == -1) {
    printf("Client::sendmsg error\n");
    return errno;
  }

  length = payload2.kremove(_socket, size2); 
  if (length==-1) {
    printf("Client::kremove error\n");
    return errno;
  }

  if (length != size2) {
    printf("Client::sendz2 %d/%d\n",length,size2);
    return ENOMEM;
  }

  return 0;
}

/*
** ++
**
**
**
** --
*/

#ifdef ODF_LITTLE_ENDIAN
void  Client::_swap(const iovec* iov, unsigned msgcount, iovec* iov_swap) {
  unsigned* dst = (unsigned*)_swap_buffer;  
  const iovec* iov_end = iov+msgcount;
  unsigned total = 0;
  do {
    unsigned  len = iov->iov_len;
    unsigned* src = (unsigned*)(iov->iov_base);
    unsigned* end = src + (len >> 2);
    while (src < end) *dst++ = htonl(*src++);
    total += len;
  } while (++iov < iov_end);
  iov_swap->iov_len = total;
  iov_swap->iov_base = _swap_buffer; 
}
#endif

#if defined(VXWORKS) && defined(DO_ZBUFS)
#warning Using ZBUFs

/*
** ++
**
**    This function is used to transmit the specified zbuffer to the
**    address specified by the "dst" argument.
**    The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/

#include <vxWorks.h>
#include <zbufSockLib.h>

int Client::send(ZBUF_ID       zId,
                    unsigned      sizeofPayload,
                    const Ins& dst)
  {
    Sockaddr sa(dst);
    int size   = sizeofDatagram() + sizeofPayload;
    int length = zbufSockSendto(_socket, zId, size, SendFlags,
				sa.name(), sa.sizeofName());
    return (length == -1) ? errno : 0;
  }
#endif // DO_ZBUFS

#if 0
/*
** ++
**
**    The idea behind _rateLimit is to provide a mechanism that ensures that
**    no more bytes are presented to the network interface than at whatever
**    rate it can handle.  If we present the VxWorks network stack with data
**    at a higher rate than what it can handle, we observe that not only are
**    packets lost, but it locks up for long periods of time (10s of seconds).
**
**    In our current situation, we are using 100 BaseT ethernet.  Since we'd
**    rather give up the CPU to another task than spin while waiting for enough
**    ethernet MTU packets to become free to send the requested bytes, we give
**    up the processor for the minimum amount of time allowed by VxWorks.  In
**    the current (default) configuration, that is 1/60 second.  At 100 Mbps
**    this corresponds to some 200 KBytes, which is about the same as the
**    currently configured number of available transmit MTU packets (144
**    packets * 1520 bytes per packet, from
**      netPoolShow(endFindByName("dc", 0)->pNetPool);
**    So, in 1 tick, all packets should become free.  However, we want to make
**    sure that we never get in a state where we're in the delay with bytes to
**    be sent, but the interface has sent all the pending bytes.
**
**    This needs more thought, but I need to get to something else right now.
**    - Size should probably be rounded up to the nearest number of MTUs and
**      include whatever overhead there is?
**    x Don't delay if the number of bytes sent since the last time is <= the
**      number of bytes being requested to be sent.
**    - Do inaccuracies in the model and calculations cause creep?
**    - Consider that Client doesn't necessarily use 100 BaseT ethernet as
**      its transport mechanism.  It could just as well use the VME backplane.
**    - Adding this routine is not backwardly compatible because the size of
**      the Client object increases due to the private member data.
**      However, since we want to treat all the traffic going through the NIC
**      and thus have only one instance of these member data per NIC,
**      independent of the number of Clients per NIC, it makes more sense to
**      put it here than in the code of users of this class, since all outgoing
**      network traffic ODF is in control of funnels through this class.
**
** --
*/

#define E100Mbps (100000000/8)          // 100 Megabits per second
#define E1Gbps   (10 * E100Mbps)        // 1   Gigabits per second
#define E10Gbps  (10 * E1Gbps)          // 10  Gigabits per second

#define TimePerTick ((unsigned)SysClk::nsPerTick())
#define NICrate     ((TimePerTick*E100Mbps)/1000000) // 1K*nS/tick*B/S = KB/tick
#define RATE        ((1024 * NICrate) / 1000)        // So we can shift

void Client::_rateLimit(unsigned size)
  {
  unsigned pending = _pending;
  unsigned last    = _last;
  unsigned current = SysClk::sample();
  unsigned dT      = current >= last ? current - last : last - current;
  unsigned sent    = (RATE * dT) >> 10; // Preserve accuracy
  if (sent <= size)
    {
    while (sent < pending)              // some pending bytes are still awaiting
      {                                 // sending (<=> 100 Mbps < pending/dT)
      pending -= sent;
      last     = current;

      taskDelay(1);                     // Give up the CPU for a minimal time
                                        // Would rather wait size/E100Mbps
      current = SysClk::sample();
      dT      = current >= last ? current - last : last - current;
      sent    = (RATE * dT) >> 10;      // Preserve accuracy

      ++_limited;
      }
    }

  // => all pending bytes have been sent
  _pending = pending > sent ? pending - sent + size : size;
  _last    = current;

  ++_sends;
  }

#endif
