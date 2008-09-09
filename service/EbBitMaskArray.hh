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
#ifndef PDS_EBBITMASKARRAY_
#define PDS_EBBITMASKARRAY_

namespace Pds {
class EbBitMaskArray
{

public :

  enum{ BitMaskWords = 2, BitMaskBits = (BitMaskWords << 5)};

  enum InitMask {FULL = -1, ONE = 1};

  inline EbBitMaskArray(void);
  inline EbBitMaskArray(const EbBitMaskArray& mask);
  inline EbBitMaskArray(InitMask mask);

  inline EbBitMaskArray operator =   (const EbBitMaskArray& mask);

  inline EbBitMaskArray operator <<  (unsigned     shift) const;
  inline EbBitMaskArray operator >>  (unsigned     shift) const;
  inline EbBitMaskArray operator <<= (unsigned     shift);
  inline EbBitMaskArray operator >>= (unsigned     shift);
  inline EbBitMaskArray operator ++  (void);
  inline EbBitMaskArray operator --  (void);

  inline EbBitMaskArray operator ~   (void)               const;

  inline EbBitMaskArray operator &   (const EbBitMaskArray& mask)  const;
  inline EbBitMaskArray operator |   (const EbBitMaskArray& mask)  const;
  inline EbBitMaskArray operator &=  (const EbBitMaskArray& mask);
  inline EbBitMaskArray operator |=  (const EbBitMaskArray& mask);

  inline EbBitMaskArray setBit       (unsigned index);
  inline EbBitMaskArray clearBit     (unsigned index);
  inline EbBitMaskArray clearAll     (void);

  inline unsigned     operator ==  (const EbBitMaskArray& mask)  const;
  inline unsigned     operator !=  (const EbBitMaskArray& mask)  const;

  inline unsigned     isZero       (void)               const;
  inline unsigned     isNotZero    (void)               const;
  inline unsigned     LSBnotZero   (void)               const;
  inline unsigned     hasBitSet    (unsigned index)     const;
  inline unsigned     hasBitClear  (unsigned index)     const;
  inline unsigned     value        (unsigned index = 0) const;

  int read(const char* arg, char** end);
  int write(char* buf) const;
  
private :

  unsigned _mask[BitMaskWords];

};
}

inline Pds::EbBitMaskArray::EbBitMaskArray(void)
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    _mask[i] = 0;
}

inline Pds::EbBitMaskArray::EbBitMaskArray(const Pds::EbBitMaskArray& mask)
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    _mask[i] = mask._mask[i];
}

inline Pds::EbBitMaskArray::EbBitMaskArray(InitMask mask)
{
  {
    switch (mask) {
    case FULL : {
      for (int i = 0; i < BitMaskWords; i++)
	_mask[i] = 0xffffffff;
      break;
    }
    case ONE : {
      _mask[0] = 1;
      for (int i = 1; i < BitMaskWords; i++)
	_mask[i] = 0;
      break;
    }
    default : {
      for (int i = 0; i < BitMaskWords; i++)
	_mask[i] = 0;
      break;
    }
    }
  }
}

inline unsigned Pds::EbBitMaskArray::value(unsigned index) const
{
  if (index < BitMaskWords)
    return _mask[index];
  return 0;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator =   (const Pds::EbBitMaskArray& mask) 
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    _mask[i] = mask._mask[i];
  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator <<  (unsigned shift) const
{
  if (!shift) return *this;

  Pds::EbBitMaskArray tempMask;
  
  if (shift < BitMaskBits) {

    unsigned wordshift = shift >> 5;
    unsigned bitshift  = shift & 0x1f;
    
    for (unsigned i = BitMaskWords; i > (wordshift + 1); i--) {

      unsigned temp = _mask[i - wordshift - 1];

      if(bitshift) {
	temp <<= bitshift;
	temp  += _mask[i - wordshift - 2] >> (32 - bitshift);
      }

      tempMask._mask[i-1] = temp;

    }

    tempMask._mask[wordshift] = (_mask[0] << bitshift);

  }
  
  return tempMask;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator >>  (unsigned shift) const
{
  if (!shift) return *this;

  Pds::EbBitMaskArray tempMask;

  if(shift < BitMaskBits) {

      unsigned wordshift = shift >> 5;
      unsigned bitshift  = shift & 0x1f;
     
      for (unsigned i = 0; i < (BitMaskWords - wordshift - 1); i++) {

	unsigned temp = _mask[i + wordshift];

	if(bitshift) {
	  temp >>= bitshift;
	  temp  += _mask[i + wordshift + 1] << (32 - bitshift);
	}

	tempMask._mask[i] = temp;

      }

      tempMask._mask[BitMaskWords - wordshift - 1] = (_mask[BitMaskWords - 1] >> bitshift);
      
  }
  
  return tempMask;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator <<= (unsigned shift)
{
  if (shift < BitMaskBits) {

    if (shift) {
      
      unsigned wordshift = shift >> 5;
      unsigned bitshift  = shift & 0x1f;
      
      for (unsigned i = BitMaskWords; i > (wordshift + 1); i--) {
	
	_mask[i-1] =  _mask[i - wordshift - 1];
	
	if(bitshift) {
	  _mask[i-1] <<= bitshift;
	  _mask[i-1]  += _mask[i - wordshift - 2] >> (32 - bitshift);
	}
	
      }
      
      _mask[wordshift] = (_mask[0] << bitshift);
      
      for (unsigned j = wordshift; j > 0; j--)
	_mask[j-1] = 0;
      
    }
    
  } else {
    
    for (unsigned i = 0; i < BitMaskWords; i++)
      _mask[i] = 0;
  }
  
  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator >>= (unsigned shift)
{
  if (shift < BitMaskBits) {

    if (shift) {

      unsigned wordshift = shift >> 5;
      unsigned bitshift  = shift & 0x1f;
      
      for (unsigned i = 0; i < (BitMaskWords - wordshift - 1); i++) {

	_mask[i] = _mask[i + wordshift];
      
	if (bitshift) {
	  _mask[i] >>= bitshift;
	  _mask[i]  += _mask[i + wordshift + 1] << (32 - bitshift);
	}
	
      }

      _mask[BitMaskWords - wordshift - 1] =  (_mask[BitMaskWords - 1] >> bitshift);
      
      for (unsigned j = (BitMaskWords - wordshift); j < BitMaskWords; j++)
	_mask[j] = 0;
      
    }

  } else {
    
    for (unsigned i = 0; i < BitMaskWords; i++)
      _mask[i] = 0;
    
  }
  
  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator ++(void)
{     
  for (unsigned i = BitMaskWords - 1; i > 0; i--)     
    _mask[i] = (_mask[i] << 1) | (_mask[i - 1] >> 31);

  _mask[0] <<= 1;
  
  return *this; 
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator --(void)
{
  for (unsigned i = 0; i < (BitMaskWords - 1); i++)
    _mask[i] = (_mask[i] >> 1) | (_mask[i+1] << 31);
  _mask[BitMaskWords - 1] >>=  1;
      
  return *this;

}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator ~(void) const
{
  Pds::EbBitMaskArray tempMask;

  for (unsigned i = 0; i < BitMaskWords; i++)
    tempMask._mask[i] = ~_mask[i];

  return tempMask;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator & (const Pds::EbBitMaskArray& mask) const
{
  Pds::EbBitMaskArray tempMask;

  for (unsigned i = 0; i < BitMaskWords; i++)
    tempMask._mask[i] =  (_mask[i] & mask._mask[i]);

  return tempMask;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator |   (const Pds::EbBitMaskArray& mask) const
{
  Pds::EbBitMaskArray tempMask;

  for (unsigned i = 0; i < BitMaskWords; i++)
    tempMask._mask[i] =  (_mask[i] | mask._mask[i]);

  return tempMask;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator &=  (const Pds::EbBitMaskArray& mask)
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    _mask[i] &= mask._mask[i];

  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::operator |=  (const Pds::EbBitMaskArray& mask)
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    _mask[i] |= mask._mask[i];

 return *this;
}

inline unsigned Pds::EbBitMaskArray::operator ==  (const Pds::EbBitMaskArray& mask) const
{
  for (unsigned i = 0; i < BitMaskWords; i++)

    if (_mask[i] != mask._mask[i]) return 0;

  return 1;
}

inline unsigned Pds::EbBitMaskArray::operator !=  (const Pds::EbBitMaskArray& mask) const
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    if (_mask[i] != mask._mask[i]) return 1;

  return 0;
}

inline unsigned Pds::EbBitMaskArray::isZero(void) const
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    if (_mask[i])
      return 0;
  return 1;
}

inline unsigned Pds::EbBitMaskArray::isNotZero(void) const
{
  for (unsigned i = 0; i < BitMaskWords; i++)
    if (_mask[i])
      return 1;
  return 0;  
}

inline unsigned Pds::EbBitMaskArray::LSBnotZero(void) const
{
  return (_mask[0] & 1);
}

inline unsigned Pds::EbBitMaskArray::hasBitSet(unsigned index) const
{
  return (_mask[index >> 5] & (1 << (index & 0x1f)));
}

inline unsigned Pds::EbBitMaskArray::hasBitClear(unsigned index) const
{
  return !hasBitSet(index);
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::setBit(unsigned index)
{
  // index / 32 integer division
  _mask[index >> 5] |= (1 << (index & 0x1f)); // 1 << (index modulo 32)
  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::clearBit(unsigned index)
{
  // index / 32 integer division
  _mask[index >> 5] &= ~(1 << (index & 0x1f)); // 1 << (index modulo 32) 
  return *this;
}

inline Pds::EbBitMaskArray Pds::EbBitMaskArray::clearAll(void)
{
  for (int i = 0; i < BitMaskWords; i++)
    _mask[i] = 0;
  return *this;
}

#endif
