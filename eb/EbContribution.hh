#ifndef Pds_Eb_EbContribution_hh
#define Pds_Eb_EbContribution_hh

#include "pds/xtc/Datagram.hh"

namespace Pds {
  namespace Eb {

    class EbContribution : public Pds::Datagram
    {
    public:
      unsigned  payloadSize()   const;
      unsigned  number()        const;
      uint64_t  numberAsMask()  const;
      uint64_t  retire()        const;
    public:
      const Pds::Datagram* datagram() const;
      Pds::Datagram*       datagram();
    };
  };
};

/*
** ++
**
**   Return the size (in bytes) of the contribution's payload
**
** --
*/

inline unsigned Pds::Eb::EbContribution::payloadSize() const
{
  return xtc.sizeofPayload();
}

/*
** ++
**
**    Return the source ID of the contributor which sent this packet...
**
** --
*/

inline unsigned Pds::Eb::EbContribution::number() const
{
  return xtc.src.log();
}

/*
** ++
**
**   Return the source ID of the contributor which sent this packet as a mask...
**
** --
*/

inline uint64_t Pds::Eb::EbContribution::numberAsMask() const
{
  return 1ull << number();
}

/*
** ++
**
**   Return the (complemented) value of the source ID.  The value is used by
**   "EbEvent" to strike down its "remaining" field which in turn signifies its
**   remaining contributors.
**
** --
*/

inline uint64_t Pds::Eb::EbContribution::retire() const
{
  return ~numberAsMask();
}

/*
** ++
**
**   Provide access to the source datagram.
**
** --
*/

inline const Pds::Datagram* Pds::Eb::EbContribution::datagram() const
{
  return this;
}

inline Pds::Datagram* Pds::Eb::EbContribution::datagram()
{
  return this;
}

#endif
