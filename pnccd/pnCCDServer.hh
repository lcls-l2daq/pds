/*
 * pnCCDServer.hh
 *
 *  Created on: May 30, 2013
 *      Author: jackp
 */

#ifndef PNCCDSERVER
#define PNCCDSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/pnCCDConfigType.hh"
#include "pds/pnccd/FrameV0.hh"
#include "pds/service/Task.hh"
#include "pds/pgp/Pgp.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/service/GenericPool.hh"
#include "pds/pnccd/pnCCDManager.hh"
#include "pds/pnccd/pnCCDConfigurator.hh"
#include "pds/pnccd/Processor.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class pnCCDTrigMonitor;
   class pnCCDServer;
   class Task;
}

class Pds::pnCCDServer
   : public EbServer,
     public EbCountSrv
{
 public:
   pnCCDServer( const Src& client, unsigned configMask=0 );
   virtual ~pnCCDServer() {}
    
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
   void setpnCCD( int fd );

   unsigned configure(pnCCDConfigType*);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned flushInputQueue(int, bool print = true);
   void     enable();
   void     disable();
   void     die();
   void     debug(unsigned d) { _debug = d; }
   unsigned debug() { return _debug; }
   void     offset(unsigned c) { _offset = c; }
   unsigned offset() { return _offset; }
   unsigned myCount() { return _count; }
   void     dumpFrontEnd();
   void     printHisto(bool);
   void     process(void);
   void     allocated();
   void     runTimeConfigName(char*);
   void     manager(pnCCDManager* m) { _mgr = m; }
   pnCCDManager* manager() { return _mgr; }
   void     firstFetch()   { _firstFetch = true; }
   void     flagTriggerError();
   void     attachTrigMonitor(pnCCDTrigMonitor* selfTrigMonitor) { _selfTrigMonitor = selfTrigMonitor; }

 public:
   static pnCCDServer* instance() { return _instance; }
//   void                laneTest();

 private:
   static pnCCDServer*            _instance;
   static void instance(pnCCDServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=4, DummySize=(1<<19)+16};
   Pds::TypeId*                   _pnCCDDataType;
   Xtc                            _xtc;
   Pds::pnCCD::pnCCDConfigurator* _cnfgrtr;
   unsigned                       _count;
   unsigned                       _payloadSize;
   unsigned                       _configMask;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   unsigned                       _quads;
   unsigned                       _quadsThisCount;
   unsigned                       _quadMask;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   Pds::Task*                     _task;
   unsigned                       _ioIndex;
   Pds::Pgp::Pgp*                 _pgp;
   unsigned*                      _dummy;
   char                           _runTimeConfigName[256];
   unsigned                       _unconfiguredErrors;
   unsigned                       _compensateNoCountReset;
   unsigned                       _ignoreCount;
   pnCCDManager*                  _mgr;
   GenericPool*                   _occPool;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _getNewComp;
   bool                           _ignoreFetch;
   pnCCDTrigMonitor*              _selfTrigMonitor;
};

#endif
