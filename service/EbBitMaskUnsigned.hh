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
#ifndef PDS_EBBITMASKUNSIGNED_
#define PDS_EBBITMASKUNSIGNED_

namespace Pds {
class EbBitMaskUnsigned
{

public :

  enum{ BitMaskWords = 1, BitMaskBits = 32};

  enum InitMask {FULL = -1, ONE = 1};

  inline EbBitMaskUnsigned(void);
  inline EbBitMaskUnsigned(const EbBitMaskUnsigned& mask);
  inline EbBitMaskUnsigned(InitMask mask);

  inline EbBitMaskUnsigned operator =   (const EbBitMaskUnsigned& mask);

  inline EbBitMaskUnsigned operator <<  (unsigned     shift) const;
  inline EbBitMaskUnsigned operator >>  (unsigned     shift) const;
  inline EbBitMaskUnsigned operator <<= (unsigned     shift);
  inline EbBitMaskUnsigned operator >>= (unsigned     shift);
  inline EbBitMaskUnsigned operator ++  (void);
  inline EbBitMaskUnsigned operator --  (void);

  inline EbBitMaskUnsigned operator ~   (void)               const;

  inline EbBitMaskUnsigned operator &   (const EbBitMaskUnsigned& mask)  const;
  inline EbBitMaskUnsigned operator |   (const EbBitMaskUnsigned& mask)  const;
  inline EbBitMaskUnsigned operator &=  (const EbBitMaskUnsigned& mask);
  inline EbBitMaskUnsigned operator |=  (const EbBitMaskUnsigned& mask);

  inline EbBitMaskUnsigned setBit       (unsigned index);
  inline EbBitMaskUnsigned clearBit     (unsigned index);
  inline EbBitMaskUnsigned clearAll     (void);

  inline unsigned     operator ==  (const EbBitMaskUnsigned& mask)  const;
  inline unsigned     operator !=  (const EbBitMaskUnsigned& mask)  const;

  inline unsigned     isZero       (void)               const;
  inline unsigned     isNotZero    (void)               const;
  inline unsigned     LSBnotZero   (void)               const;
  inline unsigned     hasBitSet    (unsigned index)     const;
  inline unsigned     hasBitClear  (unsigned index)     const;
  inline unsigned     value        (unsigned index = 0) const;
  
  int read(const char* arg, char** end);
  int write(char* buf) const;
  
private :

  unsigned _mask;

};
}

inline Pds::EbBitMaskUnsigned::EbBitMaskUnsigned(InitMask mask) 
{
  switch (mask) {
  case FULL : _mask = 0xffffffff; break;
  case ONE  : _mask = 1;          break;
  default   : _mask = 0;          break;
  }
}
  
inline unsigned Pds::EbBitMaskUnsigned::value(unsigned index) const
{
  if (index)
    return 0;
  return _mask;
}

inline unsigned Pds::EbBitMaskUnsigned::hasBitSet(unsigned index) const
{
  return (_mask & (1 << index));
}

inline unsigned Pds::EbBitMaskUnsigned::hasBitClear(unsigned index) const
{
  return !hasBitSet(index);
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::setBit(unsigned index)
{
  _mask |= (1 << index);
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::clearBit(unsigned index)
{
  _mask &= ~(1 << index);
  return *this;
}

inline Pds::EbBitMaskUnsigned::EbBitMaskUnsigned(void)  :
  _mask(0)
{}

inline Pds::EbBitMaskUnsigned::EbBitMaskUnsigned(const Pds::EbBitMaskUnsigned& mask) :
  _mask(mask._mask)
{}


inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator =   (const Pds::EbBitMaskUnsigned& mask)
{
  _mask = mask._mask;
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator <<  (unsigned shift) const
{							
  Pds::EbBitMaskUnsigned tempMask;				
  tempMask._mask = _mask << shift;			
  return tempMask;					
}							
							
inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator >>  (unsigned shift) const
{							
  Pds::EbBitMaskUnsigned tempMask;				
  tempMask._mask = _mask >> shift;			
  return tempMask;					
}							
							
inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator <<= (unsigned shift)
{							
  _mask <<= shift;					
  return *this;						
}							
							
inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator >>= (unsigned shift)
{
  _mask >>= shift;
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator ++ (void)
{
  _mask <<= 1;
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator -- (void)
{
  _mask >>= 1;
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator ~   (void) const
{
  Pds::EbBitMaskUnsigned tempMask;
  tempMask._mask = ~_mask;
  return tempMask;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator &   (const Pds::EbBitMaskUnsigned& mask) const
{
  Pds::EbBitMaskUnsigned tempMask;
  tempMask._mask = _mask & mask._mask;
  return tempMask;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator |   (const Pds::EbBitMaskUnsigned& mask) const
{
  Pds::EbBitMaskUnsigned tempMask;
  tempMask._mask = _mask | mask._mask;
  return tempMask;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator &=  (const Pds::EbBitMaskUnsigned& mask)
{
  _mask &= mask._mask;
  return *this;
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::operator |=  (const Pds::EbBitMaskUnsigned& mask)
{
  _mask |= mask._mask;
  return *this;
}

inline unsigned Pds::EbBitMaskUnsigned::operator ==  (const Pds::EbBitMaskUnsigned& mask) const
{
  return (_mask == mask._mask);
}

inline unsigned Pds::EbBitMaskUnsigned::operator !=  (const Pds::EbBitMaskUnsigned& mask) const
{
  return (_mask != mask._mask);
}

inline unsigned Pds::EbBitMaskUnsigned::isZero(void) const
{
  return (_mask == 0);
}
inline unsigned Pds::EbBitMaskUnsigned::isNotZero(void) const
{
  return (_mask != 0);
}

inline unsigned Pds::EbBitMaskUnsigned::LSBnotZero(void) const
{
  return (_mask & 1);
}

inline Pds::EbBitMaskUnsigned Pds::EbBitMaskUnsigned::clearAll(void)
{
  _mask = 0;
  return *this;
}

#endif
