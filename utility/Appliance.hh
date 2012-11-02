/*
** ++
**  Package:
**      odfUtility
**
**  Abstract:
**      Abstract-Base-Class to model the behaviour of any object
**      which can be attached to a stream.
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**
** --
*/

#ifndef PDS_APPLIANCE
#define PDS_APPLIANCE

#include "pds/utility/Transition.hh"
#include "pds/utility/Occurrence.hh"

#include "pds/service/Queue.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"

/*
** --
**
**
** ++
*/

namespace Pds {

  class Appliance : Entry
  {
  public:
    enum {DontDelete = 1};
  public:
    Appliance();
    virtual ~Appliance() {}
    typedef InDatagram* (Appliance::*pmf)(InDatagram*);
    virtual Transition* transitions(Transition*) = 0;
    virtual Occurrence* occurrences(Occurrence*);
    virtual InDatagram* events     (InDatagram*) = 0;
    virtual InDatagram* occurrences(InDatagram*);
    virtual InDatagram* markers    (InDatagram*);
    Appliance*        connect   (Appliance*);
    Appliance*        disconnect();
    int               datagrams() const;
    void              post(InDatagram*);
    void              post(Transition*);
    void              post(Occurrence*);
    Appliance*        forward();
    Appliance*        backward();
  protected:
    void         event (InDatagram*);
    InDatagram*  vector(InDatagram*);
  private:
    int          _datagrams;
    unsigned     _time;
    pmf          _handlers[Sequence::NumberOfTypes];
  };
}

typedef Pds::Appliance* create_app();
typedef void destroy_app(Pds::Appliance*);

/*
** --
**
**
** ++
*/

inline int Pds::Appliance::datagrams() const
{
  return _datagrams;
}

/*
** --
**
** insert a list on a doubly-linked list. The method has one argument:
** after - A pointer to the item AFTER which the list is to be inserted.
** 'this' appliance is treated as the head of the list being inserted.
**
** ++
*/

inline Pds::Appliance* Pds::Appliance::connect(Pds::Appliance* after)
{
  return (Pds::Appliance*) insertList(after);
}

inline Pds::Appliance* Pds::Appliance::disconnect()
{
  return (Pds::Appliance*) remove();
}

/*
** --
**
**
** ++
*/

inline Pds::InDatagram* Pds::Appliance::vector(Pds::InDatagram* input)
{
  return (this->*_handlers[input->datagram().seq.type()])(input);
}

/*
** --
**
**
** ++
*/

inline Pds::Appliance* Pds::Appliance::forward(){
  return (Pds::Appliance*) next();
}

/*
** --
**
**
** ++
*/

inline Pds::Appliance* Pds::Appliance::backward(){
  return (Pds::Appliance*) previous();
}

#endif
