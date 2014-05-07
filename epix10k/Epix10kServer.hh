/*
 * Epix10kServer.hh
 *
 *  Created on: 2014.4.15
 *      Author: jackp
 */

#ifndef EPIX10KSERVER
#define EPIX10KSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/epix10k/Epix10kManager.hh"
#include "pds/epix10k/Epix10kConfigurator.hh"
#include "pds/epix10k/Epix10kDestination.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/service/GenericPool.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
  class Epix10kServer;
  class Task;
}

class Pds::Epix10kServer
   : public EbServer,
     public EbCountSrv
{
 public:
   Epix10kServer( const Src& client, unsigned configMask=0 );
   virtual ~Epix10kServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtcTop.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtcTop; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtcTop.extent; }

   //  Eb-key interface
   EbServerDeclare;

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( ZcpFragment& , int flags ) { return 0; }
   int fetch( char* payload, int flags );
   bool more() const;

   unsigned count() const;
   void setEpix10k( int fd );

   unsigned configure(Epix10kConfigType*, bool forceConfig = false);
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
   void     manager(Epix10kManager* m) { _mgr = m; }
   Epix10kManager* manager() { return _mgr; }
   void     process(char*);
   void     allocated();
   bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
   void     runTimeConfigName(char*);
   void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }
   bool     scopeEnabled() { return _scopeEnabled; }
   EpixSampler::ConfigV1*  samplerConfig() { return _samplerConfig; }
   const Xtc&      xtcConfig() { return _xtcConfig; }
   void     maintainLostRunTrigger(bool b) { _maintainLostRunTrigger = b; }

 public:
   static Epix10kServer* instance() { return _instance; }
//   void                laneTest();

 private:
   static Epix10kServer*            _instance;
   static void instance(Epix10kServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtcTop;
   Xtc                            _xtcEpix;
   Xtc                            _xtcSamplr;
   Xtc                            _xtcConfig;
   Pds::EpixSampler::ConfigV1*     _samplerConfig;
   Pds::Epix10k::Epix10kConfigurator* _cnfgrtr;
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
   Pds::Epix10k::Epix10kDestination     _d;
   char                           _runTimeConfigName[256];
   Epix10kManager*                   _mgr;
   GenericPool*                   _occPool;
   unsigned                       _unconfiguredErrors;
   char*                          _processorBuffer;
   unsigned*                      _scopeBuffer;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _ignoreFetch;
   bool                           _resetOnEveryConfig;
   bool                           _scopeEnabled;
   bool                           _scopeHasArrived;
   bool                           _maintainLostRunTrigger;
};

#endif
