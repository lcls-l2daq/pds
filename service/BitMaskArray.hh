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
#ifndef PDS_BITMASKARRAY_
#define PDS_BITMASKARRAY_

namespace Pds {

  template <unsigned N>
  class BitMaskArray
  {

  public :

    enum{ BitMaskBits = (N << 5)};

    enum InitMask {FULL = -1, ONE = 1};

    inline BitMaskArray(void);
    inline BitMaskArray(const BitMaskArray& mask);
    inline BitMaskArray(InitMask mask);

    inline BitMaskArray operator =   (const BitMaskArray& mask);

    inline BitMaskArray operator <<  (unsigned     shift) const;
    inline BitMaskArray operator >>  (unsigned     shift) const;
    inline BitMaskArray operator <<= (unsigned     shift);
    inline BitMaskArray operator >>= (unsigned     shift);
    inline BitMaskArray operator ++  (void);
    inline BitMaskArray operator --  (void);

    inline BitMaskArray operator ~   (void)               const;

    inline BitMaskArray operator &   (const BitMaskArray& mask)  const;
    inline BitMaskArray operator |   (const BitMaskArray& mask)  const;
    inline BitMaskArray operator &=  (const BitMaskArray& mask);
    inline BitMaskArray operator |=  (const BitMaskArray& mask);

    inline BitMaskArray setBit       (unsigned index);
    inline BitMaskArray clearBit     (unsigned index);
    inline BitMaskArray clearAll     (void);

    inline unsigned     operator ==  (const BitMaskArray& mask)  const;
    inline unsigned     operator !=  (const BitMaskArray& mask)  const;

    inline unsigned     isZero       (void)               const;
    inline unsigned     isNotZero    (void)               const;
    inline unsigned     LSBnotZero   (void)               const;
    inline unsigned     hasBitSet    (unsigned index)     const;
    inline unsigned     hasBitClear  (unsigned index)     const;
    inline unsigned     value        (unsigned index = 0) const;

    void print() const;

    int read(const char* arg, char** end);
    int write(char* buf) const;
  
  private :

    unsigned _mask[N];

  };
};

template<unsigned N> inline Pds::BitMaskArray<N>::BitMaskArray(void)
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] = 0;
}

template<unsigned N> inline Pds::BitMaskArray<N>::BitMaskArray(const Pds::BitMaskArray<N>& mask)
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] = mask._mask[i];
}

template<unsigned N> inline Pds::BitMaskArray<N>::BitMaskArray(InitMask mask)
{
  {
    switch (mask) {
    case FULL : {
      for (unsigned i = 0; i < N; i++)
	_mask[i] = 0xffffffff;
      break;
    }
    case ONE : {
      _mask[0] = 1;
      for (unsigned i = 1; i < N; i++)
	_mask[i] = 0;
      break;
    }
    default : {
      for (unsigned i = 0; i < N; i++)
	_mask[i] = 0;
      break;
    }
    }
  }
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::value(unsigned index) const
{
  if (index < N)
    return _mask[index];
  return 0;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator =   (const Pds::BitMaskArray<N>& mask) 
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] = mask._mask[i];
  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator <<  (unsigned shift) const
{
  if (!shift) return *this;

  Pds::BitMaskArray<N> tempMask;
  
  if (shift < BitMaskBits) {

    unsigned wordshift = shift >> 5;
    unsigned bitshift  = shift & 0x1f;
    
    for (unsigned i = N; i > (wordshift + 1); i--) {

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

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator >>  (unsigned shift) const
{
  if (!shift) return *this;

  Pds::BitMaskArray<N> tempMask;

  if(shift < BitMaskBits) {

    unsigned wordshift = shift >> 5;
    unsigned bitshift  = shift & 0x1f;
     
    for (unsigned i = 0; i < (N - wordshift - 1); i++) {

      unsigned temp = _mask[i + wordshift];

      if(bitshift) {
	temp >>= bitshift;
	temp  += _mask[i + wordshift + 1] << (32 - bitshift);
      }

      tempMask._mask[i] = temp;

    }

    tempMask._mask[N - wordshift - 1] = (_mask[N - 1] >> bitshift);
      
  }
  
  return tempMask;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator <<= (unsigned shift)
{
  if (shift < BitMaskBits) {

    if (shift) {
      
      unsigned wordshift = shift >> 5;
      unsigned bitshift  = shift & 0x1f;
      
      for (unsigned i = N; i > (wordshift + 1); i--) {
	
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
    
    for (unsigned i = 0; i < N; i++)
      _mask[i] = 0;
  }
  
  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator >>= (unsigned shift)
{
  if (shift < BitMaskBits) {

    if (shift) {

      unsigned wordshift = shift >> 5;
      unsigned bitshift  = shift & 0x1f;
      
      for (unsigned i = 0; i < (N - wordshift - 1); i++) {

	_mask[i] = _mask[i + wordshift];
      
	if (bitshift) {
	  _mask[i] >>= bitshift;
	  _mask[i]  += _mask[i + wordshift + 1] << (32 - bitshift);
	}
	
      }

      _mask[N - wordshift - 1] =  (_mask[N - 1] >> bitshift);
      
      for (unsigned j = (N - wordshift); j < N; j++)
	_mask[j] = 0;
      
    }

  } else {
    
    for (unsigned i = 0; i < N; i++)
      _mask[i] = 0;
    
  }
  
  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator ++(void)
{     
  for (unsigned i = N - 1; i > 0; i--)     
    _mask[i] = (_mask[i] << 1) | (_mask[i - 1] >> 31);

  _mask[0] <<= 1;
  
  return *this; 
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator --(void)
{
  for (unsigned i = 0; i < (N - 1); i++)
    _mask[i] = (_mask[i] >> 1) | (_mask[i+1] << 31);
  _mask[N - 1] >>=  1;
      
  return *this;

}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator ~(void) const
{
  Pds::BitMaskArray<N> tempMask;

  for (unsigned i = 0; i < N; i++)
    tempMask._mask[i] = ~_mask[i];

  return tempMask;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator & (const Pds::BitMaskArray<N>& mask) const
{
  Pds::BitMaskArray<N> tempMask;

  for (unsigned i = 0; i < N; i++)
    tempMask._mask[i] =  (_mask[i] & mask._mask[i]);

  return tempMask;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator |   (const Pds::BitMaskArray<N>& mask) const
{
  Pds::BitMaskArray<N> tempMask;

  for (unsigned i = 0; i < N; i++)
    tempMask._mask[i] =  (_mask[i] | mask._mask[i]);

  return tempMask;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator &=  (const Pds::BitMaskArray<N>& mask)
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] &= mask._mask[i];

  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::operator |=  (const Pds::BitMaskArray<N>& mask)
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] |= mask._mask[i];

  return *this;
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::operator ==  (const Pds::BitMaskArray<N>& mask) const
{
  for (unsigned i = 0; i < N; i++)

    if (_mask[i] != mask._mask[i]) return 0;

  return 1;
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::operator !=  (const Pds::BitMaskArray<N>& mask) const
{
  for (unsigned i = 0; i < N; i++)
    if (_mask[i] != mask._mask[i]) return 1;

  return 0;
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::isZero(void) const
{
  for (unsigned i = 0; i < N; i++)
    if (_mask[i])
      return 0;
  return 1;
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::isNotZero(void) const
{
  for (unsigned i = 0; i < N; i++)
    if (_mask[i])
      return 1;
  return 0;  
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::LSBnotZero(void) const
{
  return (_mask[0] & 1);
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::hasBitSet(unsigned index) const
{
  return (_mask[index >> 5] & (1 << (index & 0x1f)));
}

template<unsigned N> inline unsigned Pds::BitMaskArray<N>::hasBitClear(unsigned index) const
{
  return !hasBitSet(index);
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::setBit(unsigned index)
{
  // index / 32 integer division
  _mask[index >> 5] |= (1 << (index & 0x1f)); // 1 << (index modulo 32)
  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::clearBit(unsigned index)
{
  // index / 32 integer division
  _mask[index >> 5] &= ~(1 << (index & 0x1f)); // 1 << (index modulo 32) 
  return *this;
}

template<unsigned N> inline Pds::BitMaskArray<N> Pds::BitMaskArray<N>::clearAll(void)
{
  for (unsigned i = 0; i < N; i++)
    _mask[i] = 0;
  return *this;
}

#endif
