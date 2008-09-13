#ifndef PDS_INXTCITERATOR
#define PDS_INXTCITERATOR

/*
** ++
**  Package:
**	Container
**
**  Abstract:
**      This class allows iteration over a collection of "InXtcs".
**      An "event" generated from DataFlow consists of data described
**      by a collection of "InXtcs". Therefore, this class is 
**      instrumental in the navigation of an event's data. The set of 
**      "InXtcs" is determined by passing (typically to the constructor)
**      a root "InXtc" which describes the collection of "InXtcs"
**      to process. This root, for example is provided by an event's
**      datagram. As this is an Abstract-Base-Class, it is expected that an 
**      application will subclass from this class, providing an implementation
**      of the "process" method. This method will be called back for each
**      "InXtc" in the collection. Note that the "InXtc" to process
**      is passed as an argument. If the "process" method wishes to abort
**      the iteration a zero (0) value is returned. In any other case,
**      the "process" method must advance the full size of payload for the
**      inXtc it is presented and return that value.  The iteration is 
**      initiated by calling the "iterate" member function.
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

#include "pds/xtc/xtc.hh"

namespace Pds {

class InDatagramIterator;

class InXtcIterator
  {
  public:
    InXtcIterator(const InXtc& xtc, InDatagramIterator* root);
    InXtcIterator() {}
   virtual ~InXtcIterator() { }
  public:
    virtual int       process(const InXtc&, InDatagramIterator*) = 0;
  public:
    int               iterate();
    int               iterate(const InXtc&, InDatagramIterator*);
    const InDatagramIterator* root()              const;
  private:
    const InXtc* _xtc;
    InDatagramIterator* _root; // Collection to process in the absence of an argument...
  };
}

/*
** ++
**
**    This constructor takes an argument the "InXtc" which defines the
**    collection to iterate over.
**
**
** --
*/

inline Pds::InXtcIterator::InXtcIterator(const Pds::InXtc& xtc,
					 Pds::InDatagramIterator* root) :
  _xtc (&xtc),
  _root(root)
  {
  } 

/*
** ++
**
**    This function will return the collection specified by the constructor.
**
** --
*/

inline const Pds::InDatagramIterator* Pds::InXtcIterator::root() const
  {
  return _root;
  } 

/*
** ++
**
**    This function will commence iteration over the collection specified
**    by the constructor. 
**
** --
*/

inline int  Pds::InXtcIterator::iterate() 
{
  return iterate(*_xtc, _root);
} 

#endif
