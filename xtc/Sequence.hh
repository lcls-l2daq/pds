/*
** ++
**  Package:
**      odfUtility
**
**  Abstract:
**     This class defines the information which "tags" a datagram (see
**     "odfDatagram.hh"). It includes a 56 bit sequence number (typically
**     derived from DataFlow's time-base), 8 bits of control information
**     and 1 bit to manage chained sets of sequences.The control information 
**     identifies the TYPE of sequence (one of four values) and for each 
**     sequence type, 32 SERVICE classes. These 63 bit values are considered 
**     to be unique over the lifetime of DataFlow and (if generated by the 
**     time-base) to increase monotically. The 63 bit is used to chain or
**     extend datagrams together to overcome transport size limitations (see
**     for example, "odfOutletWire.hh").
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_SEQUENCE
#define PDS_SEQUENCE

namespace Pds {
  class Sequence 
  {
  public:
    enum Type    {Occurrence = 0, Event = 1, Marker = 2};
    enum         {NumberOfTypes = 3};
    enum Service {NoOp    ,  L1Accept,  Service2,  Service3,
                  Service4,  Service5,  Service6,  Service7,
                  Service8,  Service9,  Service10, Service11,
                  Service12, Service13, Service14, Service15};
    enum         {NumberOfServices = 16};
    enum ShrinkFlag {NotExtended};
    enum Special {Invalid};
  public:
    Sequence() {}
    Sequence(const Sequence&);
    Sequence(const Sequence&, unsigned modify);
    Sequence(const Sequence&, ShrinkFlag);
    Sequence(Type, Service, unsigned low, unsigned high);
    Sequence(Special);
  public:
    Type     type()         const;
    Service  service()      const;
    unsigned isExtended()   const;
    unsigned isFragmented() const;
    unsigned notEvent()     const;
    unsigned low()          const;
    unsigned high()         const;
    unsigned highAll()      const;
  public:
    const Sequence& operator=(const Sequence&);
    int operator==(const Sequence&)  const;
    int operator<=(const Sequence&)  const;
    int operator>=(const Sequence&)  const;
    int operator> (const Sequence&)   const;

    int isSameEvent(const Sequence&) const;
  private:

    // Note: for performance reasons FcOutletWire makes assumptions about 
    // the bit positions listed below. If you make any changes, you will need 
    // to reflect them in FcOutletWire::forward().

    // For the same reason, EventPool::deque() makes an assumption about
    // the position of the type information.

    enum {v_cntrl    = 24, k_cntrl    = 8, m_cntrl    = ((1 << k_cntrl)-1)};
    enum {v_function = 24, k_function = 7, m_function = ((1 << k_function)-1)};
    enum {v_service  = 24, k_service  = 5, m_service  = ((1 << k_service)-1)};
    enum {v_seqType  = 28, k_seqType  = 2, m_seqType  = ((1 << k_seqType)-1)};
    enum {v_fragment = 30, k_fragment = 1, m_fragment = ((1 << k_fragment)-1)};
    enum {v_extended = 31, k_extended = 1, m_extended = ((1 << k_extended)-1)};
  public:
    enum {Extended = (m_extended << v_extended)};
    enum {Function = (m_function << v_function)};
    //    enum {EventM   = (((Event << k_service) | Service29) << v_function)};
    enum {EventM   = ((Event << k_service) << v_function)};
    enum {Control  = (m_cntrl << v_cntrl)};
  private:
    unsigned _low;
    unsigned _high;
  };
}

/*
** ++
**
**    Copy constructor...
**
** --
*/

inline Pds::Sequence::Sequence(const Pds::Sequence& input) :
  _low(input._low),
  _high(input._high)
  {
  }

/*
** ++
**
**    Copy constructor, which provides for adding or modifying the
**    high order word of the sequence. This is used, for example by
**    "OutletWireHeader" to turn on the "extended" bit. The second
**    argument provides the longword which is or'd into the high order
**    word after the first argument is copied to this object.
**
** --
*/

inline Pds::Sequence::Sequence(const Pds::Sequence& input, unsigned modify) :
  _low(input._low),
  _high(input._high | modify)
  {
  }

/*
** ++
**
**    Copy constructor, which provides for turning of the extended bit on
**    the copy.
**
** --
*/

inline Pds::Sequence::Sequence(const Pds::Sequence& input, ShrinkFlag dummy) :
  _low(input._low),
  _high(input._high & ~((unsigned) Extended))
  {
  }

/*
** ++
**
**    Constructor, which takes as arguments the sequence type, its
**    service class number and the low and high order 56 bits of its
**    sequence number. Note that it is assumed that the high order
**    longword has its high order byte zero'd. If not, these values
**    modify the control portion of the sequence.
**
** --
*/

inline Pds::Sequence::Sequence(Type type,
			       Service service, 
			       unsigned low, 
			       unsigned high) :
  _low(low),
  _high(((unsigned)type << v_seqType) | ((unsigned)service << v_service) | 
	(high & (unsigned)~(m_cntrl << v_cntrl)))
  {
  }

/*
** ++
**
**    Constructor used to create an invalid value.
**
** --
*/

inline Pds::Sequence::Sequence(Pds::Sequence::Special):
  _low((unsigned) -1),
  _high((unsigned) -1)
  {
  }

/*
** ++
**
**    This function returns the sequence type...
**
** --
*/

inline Pds::Sequence::Type Pds::Sequence::type() const
  {
  return (Type)((_high >> v_seqType) & (unsigned)m_seqType);
  }

/*
** ++
**
**   This function returns the sequence's service class number...
**
** --
*/

inline Pds::Sequence::Service Pds::Sequence::service() const
  {
  return (Service)((_high >> v_service) & (unsigned)m_service);
  }

/*
** ++
**
**    Returns a TRUE (non-zero) value if the sequence is extended, a
**    zero value if not.
**
** --
*/

inline unsigned Pds::Sequence::isExtended() const
  {
  return _high & (unsigned)Extended;
  }

/*
** ++
**
**    Returns a TRUE (non-zero) value if the sequence is NOT associated
**    with an L1Accept, a zero value if it is.
**
** --
*/

inline unsigned Pds::Sequence::notEvent() const
  {
  return (unsigned)(_high & (unsigned)Function) ^ (unsigned)EventM;
  }

/*
** ++
**
**   Returns the low-order thirty-two bits of the sequence number.
**
** --
*/

inline unsigned Pds::Sequence::low() const
  {
  return _low;
  }

/*
** ++
**
**   Returns the high-order twenty-four bits of the sequence number.
**   Note that since a longword is returned, the 24 bits are returned
**   zero extended.
**
** --
*/

inline unsigned Pds::Sequence::high() const
  {
  return _high & (unsigned)~(m_cntrl << v_cntrl);
  }

/*
** ++
**
**   Returns the high-order twenty-four bits of the sequence number.
**   Note that this function returns not only the sequence number
**   but also the control byte.
**
** --
*/


inline unsigned Pds::Sequence::highAll() const
  {
  return _high;
  }

/*
** ++
**
**    This function implements the assignment operator...
**
** --
*/

inline const Pds::Sequence& Pds::Sequence::operator=(const Pds::Sequence& input)
  {
  _low = input._low;
  _high = input._high;
  return *this;
  }

/*
** ++
**
**    This function overloads the equality operator and allows for
**    comparison of equality between this and the sequence passed
**    as an argument to the function. It returns TRUE (non-zero),
**    if the two sequences are equal, and FALSE (zero) if they are
**    not.
**
** --
*/

inline int Pds::Sequence::operator==(const Pds::Sequence& input) const
  {
  unsigned high_input = input._high & ~((unsigned) Extended);
  unsigned high       = _high       & ~((unsigned) Extended);

  return (high == high_input) && (_low == input._low);
  }

/*
** ++
**
**    This function overloads the greater than or equal operator and 
**    allows for comparison between this and the sequence passed
**    as an argument to the function. It returns TRUE (non-zero),
**    if the the sequences are >=, and FALSE (zero) if they are
**    not.
**
** --
*/

inline int Pds::Sequence::operator>=(const Pds::Sequence& input) const
  {
  unsigned input_high = input._high & ~((unsigned) Control);
  unsigned high       = _high       & ~((unsigned) Control);

  return (
	  ((high == input_high) && (_low > input._low)) ||
	  ((high > input_high)                        ) || 
	  ((_low == input._low) && 
	   ((input._high & ~((unsigned) Extended)) >=
	    (_high       & ~((unsigned) Extended)))   )
	  );
  }

/*
** ++
**
**    This function overloads the greater than operator and 
**    allows for comparison between this and the sequence passed
**    as an argument to the function. It returns TRUE (non-zero),
**    if the the sequences are >, and FALSE (zero) if they are
**    not.
**
** --
*/

inline int Pds::Sequence::operator>(const Pds::Sequence& input) const
  {
  unsigned input_high = input._high & ~((unsigned) Control);
  unsigned high       = _high       & ~((unsigned) Control);

  return (
	  ((high == input_high) && _low > input._low) ||
	  ((high > input_high)                      ) || 
	  ((_low == input._low) && 
	   ((input._high & ~((unsigned) Extended)) >
	    (_high       & ~((unsigned) Extended))) )
	  );
  }

/*
** ++
**
**    This function overloads the grater than or equal operator and 
**    allows for comparison between this and the sequence passed
**    as an argument to the function. It returns TRUE (non-zero),
**    if the the sequences are equal, and FALSE (zero) if they are
**    not.
**
** --
*/

inline int Pds::Sequence::operator<=(const Pds::Sequence& input) const
  {
  unsigned input_high = input._high & ~((unsigned) Control);
  unsigned high       = _high       & ~((unsigned) Control);

  return (
	  ((high == input_high) && (_low < input._low)) ||
	  ((high < input_high)                        ) || 
	  (_low == input._low)  &&
	  ((input._high & ~((unsigned) Extended)) >
	   (_high       & ~((unsigned) Extended))     )
	  );
  }

/*
**  Condition for combining contributions seen by the event builder.
*/
inline int Pds::Sequence::isSameEvent(const Pds::Sequence& input) const
  {
    return (*this)==input;
  }

#endif
