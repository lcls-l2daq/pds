#ifndef XAMPSSERVER
#define XAMPSSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/XampsConfigType.hh"
#include "pds/service/Task.hh"
//#include "pdsdata/xamps/ElementHeader.hh"
#include "pds/xamps/XampsConfigurator.hh"
#include "pds/xamps/XampsDestination.hh"
#include "pds/xamps/Processor.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class XampsServer;
   class Task;
}

class Pds::XampsServer
   : public EbServer,
     public EbCountSrv
{
 public:
   XampsServer( const Src& client, unsigned configMask=0 );
   virtual ~XampsServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtc.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtc; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtc.extent; }

   //  Eb-key interface
   EbServerDeclare;

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( char* payload, int flags );
   bool more() const;

   unsigned count() const;
   void setXamps( int fd );

   unsigned configure(XampsConfigType*);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned numberOfElements(void) { return _elements; }
   unsigned flushInputQueue(int);
   void     enable();
   void     disable();
   void     die();
   void     debug(unsigned d) { _debug = d; }
   unsigned debug() { return _debug; }
   void     offset(unsigned c) { _offset = c; }
   unsigned offset() { return _offset; }
   void     resetOffset() { _offset = 0; _count = 0xffffffff; }
   unsigned myCount() { return _count; }
   void     dumpFrontEnd();
   void     printHisto(bool);
   void     process(void);

 public:
   static XampsServer* instance() { return _instance; }
//   void                laneTest();

 private:
   static XampsServer*            _instance;
   static void instance(XampsServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=4};
   Xtc                            _xtc;
   Pds::Xamps::XampsConfigurator* _cnfgrtr;
   unsigned                       _elements;
   unsigned                       _count;
   unsigned                       _elementsThisCount;
   unsigned                       _payloadSize;
   unsigned                       _configMask;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   Pds::Task*                     _task;
   unsigned                       _ioIndex;
   Pds::Xamps::XampsDestination   _d;
   Pds::Pgp::Pgp*                 _pgp;
   unsigned                       _unconfiguredErrors;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _iHaveLaneZero;
};

#endif
