/*
 * EpixServer.hh
 *
 *  Created on: 2013.11.9
 *      Author: jackp
 */

#ifndef EPIXSERVER
#define EPIXSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/epix/EpixManager.hh"
#include "pds/epix/EpixConfigurator.hh"
#include "pds/epix/EpixDestination.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/service/GenericPool.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
  class EpixServer;
  class Task;
}

class Pds::EpixServer
   : public EbServer,
     public EbCountSrv
{
 public:
   EpixServer( const Src& client, unsigned configMask=0 );
   virtual ~EpixServer() {}
    
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
   void setEpix( int fd );

   unsigned configure(EpixConfigType*, bool forceConfig = false);
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
   void     clearHisto();
   void     manager(EpixManager* m) { _mgr = m; }
   EpixManager* manager() { return _mgr; }
   void     process(char*);
   void     allocated();
   bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
   void     runTimeConfigName(char*);
   void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }

 public:
   static EpixServer* instance() { return _instance; }
//   void                laneTest();

 private:
   static EpixServer*            _instance;
   static void instance(EpixServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtc;
   Pds::Epix::EpixConfigurator* _cnfgrtr;
   unsigned                       _elements;
   unsigned                       _count;
   unsigned                       _elementsThisCount;
   unsigned                       _payloadSize;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   Pds::Task*                     _task;
   unsigned                       _ioIndex;
   Pds::Epix::EpixDestination     _d;
   char                           _runTimeConfigName[256];
   EpixManager*                   _mgr;
   GenericPool*                   _occPool;
   unsigned                       _unconfiguredErrors;
   char*                          _processorBuffer;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _ignoreFetch;
   bool                           _resetOnEveryConfig;
};

#endif
