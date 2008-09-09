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

#ifndef PDS_PORT
#define PDS_PORT

#ifdef VXWORKS
#  include <sockLib.h>
#  include <inetLib.h>
#  include <hostLib.h> 
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netdb.h>
#endif

#include "Ins.hh"

namespace Pds {
class Port : public Ins
  {
  public:
    enum Type {ClientPort = 0, ServerPort = 2, VectoredServerPort = 3};
    ~Port();
    Port(Port::Type, 
            const Ins&, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    Port(Port::Type, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    int  sizeofDatagram() const;
    int  maxPayload()     const;
    int  maxDatagrams()   const;
    Type type()           const;

    // Construction may potentially fail. If the construction is
    // successful error() returns 0. If construction fails, error()
    // returns a non-zero value which corresponds to the reason (as an
    // "errno").
    int  error() const;
  protected:
    void error(int value);
    int _socket;
  private:
    enum {Class_Server = 2, Style_Vectored = 1};
    void   _open(Port::Type, 
		 const Ins&, 
		 int sizeofDatagram, 
		 int maxPayload,
		 int maxDatagrams);
    int    _bind(const Ins&);
    void   _close();
    Type   _type;
    int    _sizeofDatagram;
    int    _maxPayload;
    int    _maxDatagrams;
    int    _error;
  };
}
/*
** ++
**
**
** --
*/

inline Pds::Port::~Port() 
  {
  _close();
  }

/*
** ++
**
**
** --
*/

inline Pds::Port::Port(Pds::Port::Type type, 
                        int sizeofDatagram, 
                        int maxPayload,
                        int maxDatagrams) : 
  Pds::Ins(Pds::Ins::DoNotInitialize)
  {
  Pds::Ins ins; _open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
  }

/*
** ++
**
**
** --
*/

inline Pds::Port::Port(Pds::Port::Type type, 
		       const Pds::Ins& ins, 
		       int sizeofDatagram, 
		       int maxPayload,
		       int maxDatagrams) : 
Ins(Ins::DoNotInitialize)
{
_open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
}

/*
** ++
**
**
** --
*/

inline Pds::Port::Type Pds::Port::type() const
  {
  return _type;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Port::sizeofDatagram() const
  {
  return _sizeofDatagram;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Port::maxPayload() const
  {
  return _maxPayload;
  }


/*
** ++
**
**
** --
*/

inline int Pds::Port::maxDatagrams() const
  {
  return _maxDatagrams;
  }

/*
** ++
**
**
** --
*/

inline int Pds::Port::error() const
  {
  return _error;
  }

/*
** ++
**
**
** --
*/

inline void Pds::Port::error(int value)
  {
  _error = value;
  }

#endif
