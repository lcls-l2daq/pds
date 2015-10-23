/*
** ++
**  Package:
**	Services
**
**  Abstract:
**      Non-inline functions for class "Server.hh"
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

#include <errno.h>
#include <string.h>

#include <strings.h>

#include "NetServer.hh"
#include "Sockaddr.hh"

using namespace Pds;

/*
** ++
**
**   Internal function called back from the constructor described below.
**   Used mostly to set up the structures necessary to issue split reads
**   to socket API. Our interface arbitrary breaks up all arrived datagrams
**   into two pieces, A fixed size header (called the datagram) and a variable
**   sized trailer (called the payload). The size of the datagram and the
**   MAXIMUM size of the payload are determined by arguments passed to this
**   function. Note, that a buffer (internal to this class) is allocated for
**   the datagram.
**
** --
*/

void NetServer::_construct(int sizeofDatagram, int maxPayload)
  {
  memset((void*)&_hdr, 0, sizeof(_hdr));

  _hdr.msg_name         = (caddr_t)&_src;
  _hdr.msg_namelen      = sizeof(_src);
  _hdr.msg_iov          = &_iov[0];

  _hdro = _hdr;

  if((_sizeofDatagram = sizeofDatagram))
    {
    _iov[0].iov_len  = sizeofDatagram;
    char* datagram   = new char[sizeofDatagram];
    _datagram        = datagram;
    _iov[0].iov_base = datagram;
    // JT 11/9/09: This is a hack to quiet gcc's overly-anal
    // "type-punned" alias warning.
    // _payload         = reinterpret_cast<char**>(&_iov[1].iov_base);
    _payload         = (char**)(void*)(&_iov[1].iov_base);
    _iov[1].iov_len  = maxPayload;
    _hdr.msg_iovlen  = (_maxPayload = maxPayload) ? 2 : 1;
    }
  else
    {
    // _payload         = reinterpret_cast<char**>(&_iov[0].iov_base);
    _payload         = (char**)(void*)(&_iov[0].iov_base);
    _hdr.msg_iovlen  = 1;
    _datagram        = (char*)0;
    _iov[0].iov_len  = maxPayload;
    _maxPayload      = maxPayload;
    }

#ifdef ODF_LITTLE_ENDIAN
  _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif

  }

/*
** ++
**
**   Constructor for server has as arguments the size of a datagram (in bytes),
**   the maximum payload size (in bytes), and the maximum number of datagrams
**   which can be pending in the underlying network queue at any given time.
**   Together, these three arguments determine how much receive buffering
**   should be allocated for the port associated with this server.
**
** --
*/

NetServer::NetServer(unsigned id,
		     int sizeofDatagram,
		     int maxPayload,
		     int maxDatagrams) :
  Port(Port::VectoredServerPort, sizeofDatagram, maxPayload, maxDatagrams),
  OobServer(id, socket())
  {
  _construct(sizeofDatagram, maxPayload);
  }

/*
** ++
**
**    This constructor operates as described above, with the exception, that
**    rather than allocating a scratch port, the port is explicitly passed
**    by the user as an argument.
**
** --
*/

NetServer::NetServer(unsigned id,
                     const Ins& ins,
                     int sizeofDatagram,
                     int maxPayload,
                     int maxDatagrams) :
  Port(Port::VectoredServerPort, ins,
       sizeofDatagram,
       maxPayload,
       maxDatagrams),
  OobServer(id, socket())
  {
  _construct(sizeofDatagram, maxPayload);
  }

/*
** ++
**
**    This constructor operates as described above, with the exception, that
**    rather than allocating a scratch port, the port is explicitly passed
**    by the user as an argument.
**
** --
*/

NetServer::NetServer(unsigned id,
                     Port::Type,
                     const Ins& ins,
                     int sizeofDatagram,
                     int maxPayload,
                     int maxDatagrams) :
  Port(Port::VectoredServerPort,
       ins,
       sizeofDatagram,
       maxPayload,
       maxDatagrams),
  OobServer(id, socket())
  {
  _construct(sizeofDatagram, maxPayload);
  }

/*
** ++
**
**    This function is used to hang a read on the port associated with the
**    server. The function first allocates a payload buffer (by calling the
**    derived class's "payload" member function and than hangs the read.
**    Once the read is satisfied, the derived classes "commit" function is
**    called passing to it the arrived datagram as two pieces, the header (or
**    datagram) and the trailer (or payload) and its size. In addition, the
**    "commit" function is passed the IP address of the the client which sent
**    the datagram. Any transport errors are handled by calling the derived
**    class's "handleError" function, passing the error number "errno" as an
**    argument. 
**    The return values of commit and handleError form the return value of
**    pend(). This is used by ManagedNetServer to determine wether an 
**    attempt should be made to read further data from the socket(non-zero), 
**    or if treatment of this socket should be completed (potentially 
**    to re-call select()).   
**
** --
*/

#include <stdio.h>

int NetServer::fetch      (char* payload, int flags)
  {
  *_payload  = payload;

  int length = recvmsg(_socket, &_hdr, flags);

  if(length >= 0)
    {
#ifdef ODF_LITTLE_ENDIAN
    _swap(length);
#endif
    length -= sizeofDatagram();
    if(length < 0) length = 0;
    }
  else
    {
      printf("NetServer::fetch failed payload %p  flags %x  socket %d\n",
	     payload, flags, _socket);
    handleError(errno);
    }

  return length;
  }

/*
** ++
**
**    This function is used to hang a read on the port associated with the
**    server. The function first allocates a payload buffer (by calling the
**    derived class's "payload" member function and than hangs the read.
**    Once the read is satisfied, the derived classes "commit" function is
**    called passing to it the arrived datagram as two pieces, the header (or
**    datagram) and the trailer (or payload) and its size. In addition, the
**    "commit" function is passed the IP address of the the client which sent
**    the datagram. Any transport errors are handled by calling the derived
**    class's "handleError" function, passing the error number "errno" as an
**    argument. 
**    The return values of commit and handleError form the return value of
**    pend(). This is used by ManagedNetServer to determine wether an 
**    attempt should be made to read further data from the socket(non-zero), 
**    or if treatment of this socket should be completed (potentially 
**    to re-call select()).   
**
** --
*/

int NetServer::pend(int flags)
  {
  char* body = payload();
  *_payload  = body;

  int length = recvmsg(_socket, &_hdr, flags);

  if (!length)  return handleError(EWOULDBLOCK);

  int repend;

  if(length != - 1)
    {
#ifdef ODF_LITTLE_ENDIAN
    _swap(length);
#endif
    length -= sizeofDatagram();
    if(length < 0) length = 0;
    Ins src(_src);
    repend = commit(_datagram, body, length, src);
    }
  else
    repend = handleError(errno);

  return repend;
  }

/*
**    Default implementations which do nothing.
**
*/

char* NetServer::payload()
{
  return (char*)0;
}

int NetServer::commit(char* key,
		      char* payload,
		      int sizeofPayload,
		      const Ins&)
{
  return 1;
}


int NetServer::handleError(int value)
{
  printf("*** NetServer::handleError error  socket %d: %s\n",
         _socket, strerror(value));
  return 1;
}

/*
** ++
**
**    Send a message to ourselves. The message to be sent is specified by
**    the input arguments. If the "payload" and "sizeofPayload" arguments
**    are not supplied only the header (datagram) portion of the message
**    is sent. The function returns a value of zero (0) if successful.
**    If not successful, the return value corresponds to the error number
**    (errno).
**
** --
*/

int NetServer::unblock(char* datagram)
{
  struct msghdr hdr;
  struct iovec  iov[1];

  iov[0].iov_base = (caddr_t)datagram;
  iov[0].iov_len  = sizeofDatagram();

  Sockaddr sa(*this);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;
  hdr.msg_iovlen    = 1;
  hdr.msg_iov       = &iov[0];
  hdr.msg_flags     = 0;

  return sendmsg(_socket, &hdr, SendFlags) != - 1 ? 0 : errno;
  }

int NetServer::unblock(char* datagram, char* payload, int sizeofPayload)
{
  struct msghdr hdr;
  struct iovec  iov[2];

  iov[0].iov_base = (caddr_t)datagram;
  iov[0].iov_len  = sizeofDatagram();
  iov[1].iov_base = (caddr_t)payload;
  iov[1].iov_len  = sizeofPayload;

  Sockaddr sa(*this);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;
  hdr.msg_iovlen    = 2;
  hdr.msg_iov       = &iov[0];
  hdr.msg_flags     = 0;

  return sendmsg(_socket, &hdr, SendFlags) != - 1 ? 0 : errno;
}

/*
** ++
**
**    The destructor for this class deallocates the buffer used to
**    contain the datagram...
**
** --
*/

NetServer::~NetServer()
  {
    resign();
    delete [] _datagram;
#ifdef ODF_LITTLE_ENDIAN
    delete [] _swap_buffer;
#endif
  }

/*
** ++
**
** Tell the server to listen for messages from a specified multicast group
** on the specified interface. A return value of zero indicates success, otherwise
** the errno value which results from the underlying setsockopt call is 
** returned.
**
** --
*/

int NetServer::join(const Ins& group, const Ins& interface){
  McastSubscription s;
  s.group     = group    .address();
  s.interface = interface.address();
  _mcasts.push_back(s);
  struct ip_mreq ipMreq;
  bzero ((char*)&ipMreq, sizeof(ipMreq));
  ipMreq.imr_multiaddr.s_addr = htonl(group    .address());
  ipMreq.imr_interface.s_addr = htonl(interface.address());
  return (setsockopt (_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipMreq,
		      sizeof(ipMreq)) < 0) ? errno : 0; 
}

/*
** ++
**
** Tell the server to stop listening for messages from a specified
** multicast group on the specified interface. A return value of zero
** indicates success, otherwise the errno value which results from the
** underlying setsockopt call is returned.
**
** --
*/

int NetServer::resign(){
  for(std::list<McastSubscription>::iterator it = _mcasts.begin();
      it != _mcasts.end(); it++) {
    struct ip_mreq ipMreq;
    bzero ((char*)&ipMreq, sizeof(ipMreq));
    ipMreq.imr_multiaddr.s_addr = htonl(it->group    );
    ipMreq.imr_interface.s_addr = htonl(it->interface);
    int result = (setsockopt (_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&ipMreq,
			      sizeof(ipMreq)) < 0) ? errno : 0;  
    if (result) return result;
  }
  _mcasts.clear();
  return 0;
}

/*
** ++
**
** Perform swap in place of the data contained in the iov. Assumes all
** data is four bytes aligned.
**
** --
*/

#ifdef ODF_LITTLE_ENDIAN
void NetServer::_swap(int length) {
  const iovec* iov = _iov;
  const iovec* iov_end = iov+_hdr.msg_iovlen;
  do {
    unsigned  len = (length < iov->iov_len) ? length : iov->iov_len;
    unsigned* buf = (unsigned*)(iov->iov_base);
    unsigned* end = buf + (len >> 2);
    while (buf < end) {
      *buf = ntohl(*buf); 
      buf++;
    }
    length -= len;
  } while (++iov < iov_end && length > 0);
}
#endif
