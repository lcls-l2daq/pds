/*
** ++
**  Package:
**	Services
**
**  Abstract:
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

#include "Port.hh"
#include "Sockaddr.hh"
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

using namespace Pds;

/*
** ++
**
**
** --
*/

void Port::_open(Port::Type type,
                    const Ins& ins,
                    int sizeofDatagram,
                    int maxPayload,
                    int maxDatagrams)
{
  _socket         = socket(AF_INET, SOCK_DGRAM, 0);
  _sizeofDatagram = sizeofDatagram;
  _maxPayload     = maxPayload;
  _maxDatagrams   = maxDatagrams;
  _type           = type;
  if(!(_error = (_socket != -1) ? _bind(ins) : errno))
    {
      sockaddr_in name;
#ifdef VXWORKS
      int length = sizeof(name);
#else
      unsigned length = sizeof(name);
#endif
      if(getsockname(_socket, (sockaddr*)&name, &length) == 0) {
	_address = ntohl(name.sin_addr.s_addr);
	_port = (unsigned)ntohs(name.sin_port);
      } else {
	_error = errno;
	_close();
      }
    }
  else
    _close();
  if (_error) {
    printf("*** Port failed to open address 0x%x port %i: %s\n", 
	   ins.address(), ins.portId(), strerror(errno));
  }
}

/*
** ++
**
**
** --
*/

int Port::_bind(const Ins& ins)
  {
  int s = _socket;

  int yes = 1;
  if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes)) == -1)
    return errno;

  if(_type == Port::ClientPort)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;

#ifdef VXWORKS                           // Out for Tornado II 7/21/00 - RiC
    // The following was found exprimentally with ~claus/bbr/test/sizeTest
    // The rule may well change if the mBlk, clBlk or cluster parameters change
    // as defined in usrNetwork.c - RiC
    parm = parm + (88 + 32 * ((parm - 1993) / 2048));
#endif

    /*
     * Set the send buffer size to be small (32K), since we
     * have done the traffice shaping by ourselves
     */
    parm = 32*1024; 
    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    }

  if(_type & Class_Server)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;
#ifdef __linux__
    parm += 4096; // 1125
#endif
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    if(ins.portId() && (_type & Style_Vectored))
      {
      int y = 1;
#ifdef VXWORKS 
      if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char*)&y, sizeof(y)) == -1)
#else
      if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1)
#endif
        return errno;
      }
    }

#ifdef VXWORKS
  Sockaddr sa(Ins(ins.portId()));
#else
  Sockaddr sa(ins);
#endif
  return (bind(s, sa.name(), sa.sizeofName()) == -1) ? errno : 0;
  }

/*
** ++
**
**
** --
*/

void Port::_close()
  {
  if(_socket != -1)
    {
    close(_socket);

    _socket = -1;
    }
  }
