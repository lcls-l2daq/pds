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

#ifndef PDS_INS
#define PDS_INS

#ifdef VXWORKS
#include <inetLib.h>
#else
#include <arpa/inet.h>
#endif

#if (defined _XOPEN_SOURCE && _XOPEN_SOURCE - 500 != 0)
#define explicit
#endif

namespace Pds {
class Ins
  {
  public:
    Ins();
    enum Option {DoNotInitialize};
    explicit Ins(Option);
    explicit Ins(unsigned short port);
    explicit Ins(int address);
    Ins(int address, unsigned short port);
    Ins(const Ins& ins, unsigned short port);
    explicit Ins(const sockaddr_in& sa);

    int                   address()     const; 
    void                  address(int address);
    unsigned short        portId()      const; 

    int operator==(const Ins& that)  const;

  protected:
    int      _address;
    unsigned _port;
  };
}

/*
** ++
**
**
** --
*/

inline int Pds::Ins::operator==(const Pds::Ins& that) const{
  return (_address == that._address && _port == that._port);
}

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(Option) 
  {
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins() 
  {
    _address = INADDR_ANY;
    _port    = 0;
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(unsigned short port) 
  {
    _address = INADDR_ANY;
    _port    = port;
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(int address, unsigned short port) 
  {
    _address = address;
    _port    = port;
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(const Pds::Ins& ins, unsigned short port) 
  {
    _address = ins._address;
    _port    = port;
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(const sockaddr_in& sa) 
  {
    _address = ntohl(sa.sin_addr.s_addr);
    _port    = ntohs(sa.sin_port);
  }

/*
** ++
**
**
** --
*/

inline Pds::Ins::Ins(int address) 
  {
    _address = address;
    _port    = 0;
  }

/*
** ++
**
**
** --
*/

inline unsigned short Pds::Ins::portId() const 
  {
    return _port;  
  }

/*
** ++
**
**
** --
*/

inline void Pds::Ins::address(int address) 
  {
    _address = address;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Ins::address() const 
  {
    return _address;
  }
#endif
