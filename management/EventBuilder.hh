#ifndef EventBuilder_hh
#define EventBuilder_hh

//#if ( __GNUC__ > 3 )
#ifdef BUILD_ZCP

#include "pds/utility/ZcpEbS.hh"
typedef Pds::ZcpEbS EventBuilder;

#include "pds/utility/ZcpEbC.hh"
typedef Pds::ZcpEbC L1EventBuilder;

#else

#include "pds/utility/EbS.hh"
typedef Pds::EbS EventBuilder;

#include "pds/utility/EbC.hh"
typedef Pds::EbC L1EventBuilder;

#endif

#endif
