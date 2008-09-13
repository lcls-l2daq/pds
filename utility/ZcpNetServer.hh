/*
** ++
**  Package:
**	Services
**
**  Abstract:
**      This class wraps the "C" socket (UDP receiving side) API into
**      something more palatable to our OOP world. In general, the "pend"
**      function is called to hang a read on a socket (in our paralence
**      a "port"). Hanging the read causes memory to be allocated (through
**      the virtual function "payload"). Once the read is satisfied, the
**      virtual function "commit" is called to process the arrived data.
**      Any transport errors are handled through the "handleError" function.
**      Note: that by convention, all arrived data is arbitrarily broken
**      into a top (the datagram) and a bottom (the payload) piece. The
**      divisions for these pieces are defined as arguments to the constructor.
**      To assist the user in developing asynchronous applications, the
**      "unblock" function exists to send a message to ourselves. The
**      corresponding UDP transmission class is "Client". To manage
**      and multiplex multiple servers together coherently see the class
**      "ServerManager".
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

#ifndef PDS_SERVER
#define PDS_SERVER

#include "Port.hh"
#include "LinkedList.hh"

#ifdef VXWORKS
#  include <ioLib.h>
#endif

namespace Pds {

class ZcpDatagram;

class Server : public LinkedList<Server>, public Port
  {
  public:
    virtual ~Server();
  public:
    Server(unsigned id,
	   const Ins&,
	   int sizeofDatagram,
	   int maxPayload,
	   int maxDatagrams = 40);
    Server(unsigned id,
	   int sizeofDatagram,
	   int maxPayload,
	   int maxDatagrams = 40);
  public:
    int      pend(int flag = 0);
    int      fetch(char* payload, int flags);
    //    int      fetch(ZcpDatagram& , int flags);
    int      unblock(char* datagram, char* payload=(char*)0, int size=0);
    int      socket() const;
    unsigned id()     const;
    char*    datagram();
  public:
    int      join(const Ins& group, const Ins& interface);
    int      resign(const Ins& group, const Ins& interface);
  public:
    virtual char* payload()                 = 0;
    virtual int   commit(char* datagram,
                         char* payload,
                         int payloadSize,
                         const Ins&)     = 0;
    virtual int   handleError(int value)    = 0;
  protected:
    Server(unsigned id,
              Port::Type,
              const Ins&,
              int sizeofDatagram,
              int maxPayload,
              int maxDatagrams);
  private:
    enum {SendFlags = 0};
  private:
    void _construct(int sizeofDatagram, int maxPayload);
  private:
    unsigned           _socketBase;     // Byte offset of socket number and...
    unsigned           _socketMask;     // bit offset (as mask) of same
    unsigned           _id;             // Policy "neutral" identification.
    char*              _datagram;       // -> buffer for current  datagram
    char**             _payload;        // Pointer to -> buffer current payload
    struct msghdr      _hdr;            // Control structure socket receive 
    struct msghdr      _zhdr;           // Control structure socket receive for zero-copy fetches
    struct iovec       _iov[2];         // Buffer description socket receive 
    struct sockaddr_in _src;            // Socket name source machine
    int                _maxPayload;     // Maximum payload size
    int                _sizeofDatagram; // Size of datagram
#ifdef ODF_LITTLE_ENDIAN
    char* _swap_buffer;
    void _swap(int length);
#endif
  };
}

/*
** ++
**
**    Returns the file descriptor (as an integer) corresponding to
**    the server's port. In general, this function is ONLY useful
**    internally to the "ManagedServers" class, where it is used
**    in conjunction with its "select" method. Therefore, its usage
**    is not encouraged.
**
** --
*/

inline int Pds::Server::socket() const
  {
  return (int)_socket;
  }

/*
** ++
**
**    Returns the value passed as the "id" argument passed to the server's
**    constructor. Note; that the usage of this value is completely unspecified
**    by this class. In effect, it just provides a way to "label, in a policy
**    free way the server.
**
** --
*/

inline unsigned Pds::Server::id() const
  {
  return _id;
  }

/*
** ++
**
**    Return a pointer to the header (top) portion of the last received
**    datagram.
**
** --
*/

inline char* Pds::Server::datagram() 
  {
  return _datagram;
  }

#endif
