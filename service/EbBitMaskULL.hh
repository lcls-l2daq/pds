/*
** ++
**  Package:
**	Utility
**
**  Abstract:
**      Class provides a bit mask of the event builders,
**      hiding the user from the implementation details
**      and (hopefully) removing direct dependance on a
**      fixed number of nodes (ie 32)
**
**  Author:
**      jswain@ph.ed.ac.uk
**
**  Creation Date:
**	000 - May 15, 2001
**
**  Revision History:
**	001 - June 13, 2001 made VxWorks an unsigned after
**            localising the required 64 bit mask on VxWorks
**            to AckHandler
**      002 - July 11, 2001 reintroduce the array bit-mask
**      003 - July 17, 2001 add "timeout" methods.
**
** --
*/
#ifndef PDS_EBBITMASKULL_
#define PDS_EBBITMASKULL_

namespace Pds {
class EbBitMaskULL
{

public :

  // Although this mask only has _one_ unsigned long long,
  // for consistency in printout code value(index) returns this 
  // as _two_ unsigneds like an array.
  enum{ BitMaskWords = 2, BitMaskBits = 64};

  enum InitMask {FULL = -1, ONE = 1};

  inline EbBitMaskULL(void);
  inline EbBitMaskULL(const EbBitMaskULL& mask);
  inline EbBitMaskULL(InitMask mask);

  inline EbBitMaskULL operator =   (const EbBitMaskULL& mask);

  inline EbBitMaskULL operator <<  (unsigned     shift) const;
  inline EbBitMaskULL operator >>  (unsigned     shift) const;
  inline EbBitMaskULL operator <<= (unsigned     shift);
  inline EbBitMaskULL operator >>= (unsigned     shift);
  inline EbBitMaskULL operator ++  (void);
  inline EbBitMaskULL operator --  (void);

  inline EbBitMaskULL operator ~   (void)               const;

  inline EbBitMaskULL operator &   (const EbBitMaskULL& mask)  const;
  inline EbBitMaskULL operator |   (const EbBitMaskULL& mask)  const;
  inline EbBitMaskULL operator &=  (const EbBitMaskULL& mask);
  inline EbBitMaskULL operator |=  (const EbBitMaskULL& mask);

  inline EbBitMaskULL setBit       (unsigned index);
  inline EbBitMaskULL clearBit     (unsigned index);
  inline EbBitMaskULL clearAll     (void);

  inline unsigned     operator ==  (const EbBitMaskULL& mask)  const;
  inline unsigned     operator !=  (const EbBitMaskULL& mask)  const;

  inline unsigned     isZero       (void)               const;
  inline unsigned     isNotZero    (void)               const;
  inline unsigned     LSBnotZero   (void)               const;
  inline unsigned     hasBitSet    (unsigned index)     const;
  inline unsigned     hasBitClear  (unsigned index)     const;
  inline unsigned     value        (unsigned index = 0) const;
  
  int read(const char* arg, char** end);
  int write(char* buf) const;
  
private :

  unsigned long long _mask;

};
}

inline Pds::EbBitMaskULL::EbBitMaskULL(InitMask mask)
{
  switch (mask){
  case FULL : _mask = 0xffffffff; _mask |= (_mask << 32); break;
  case ONE  : _mask = 1; break;
  default   : _mask = 0; break;
  }
}
  
inline unsigned Pds::EbBitMaskULL::value(unsigned index) const
{
  if (!index)
    return (unsigned)(_mask & 0xffffffff);
  else if (index == 1)
    return (unsigned)((_mask >> 32) & 0xffffffff);
  return 0;
}

inline unsigned Pds::EbBitMaskULL::hasBitSet(unsigned index) const
{
  return ((_mask & (1ull << index)) != 0);
}

inline unsigned Pds::EbBitMaskULL::hasBitClear(unsigned index) const
{
  return ((_mask & (1ull << index)) == 0);
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::setBit(unsigned index)
{
  
  _mask |= (1ull << index);
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::clearBit(unsigned index)
{
  _mask &= ~(1ull << index);
  return *this;
}

inline Pds::EbBitMaskULL::EbBitMaskULL(void)  :
  _mask(0)
{}

inline Pds::EbBitMaskULL::EbBitMaskULL(const EbBitMaskULL& mask) :
  _mask(mask._mask)
{}


inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator =   (const EbBitMaskULL& mask)
{
  _mask = mask._mask;
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator <<  (unsigned shift) const
{							
  Pds::EbBitMaskULL tempMask;				
  tempMask._mask = _mask << shift;			
  return tempMask;					
}							
							
inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator >>  (unsigned shift) const
{							
  Pds::EbBitMaskULL tempMask;				
  tempMask._mask = _mask >> shift;			
  return tempMask;					
}							
							
inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator <<= (unsigned shift)
{							
  _mask <<= shift;					
  return *this;						
}							
							
inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator >>= (unsigned shift)
{
  _mask >>= shift;
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator ++ (void)
{
  _mask <<= 1;
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator -- (void)
{
  _mask >>= 1;
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator ~   (void) const
{
  Pds::EbBitMaskULL tempMask;
  tempMask._mask = ~_mask;
  return tempMask;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator &   (const EbBitMaskULL& mask) const
{
  Pds::EbBitMaskULL tempMask;
  tempMask._mask = _mask & mask._mask;
  return tempMask;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator |   (const EbBitMaskULL& mask) const
{
  Pds::EbBitMaskULL tempMask;
  tempMask._mask = _mask | mask._mask;
  return tempMask;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator &=  (const EbBitMaskULL& mask)
{
  _mask &= mask._mask;
  return *this;
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::operator |=  (const EbBitMaskULL& mask)
{
  _mask |= mask._mask;
  return *this;
}

inline unsigned Pds::EbBitMaskULL::operator ==  (const EbBitMaskULL& mask) const
{
  return (_mask == mask._mask);
}

inline unsigned Pds::EbBitMaskULL::operator !=  (const EbBitMaskULL& mask) const
{
  return (_mask != mask._mask);
}

inline unsigned Pds::EbBitMaskULL::isZero(void) const
{
  return (_mask == 0);
}
inline unsigned Pds::EbBitMaskULL::isNotZero(void) const
{
  return (_mask != 0);
}

inline unsigned Pds::EbBitMaskULL::LSBnotZero(void) const
{
  return (_mask & 1);
}

inline Pds::EbBitMaskULL Pds::EbBitMaskULL::clearAll(void)
{
  _mask = 0;
  return *this;
}

#endif
