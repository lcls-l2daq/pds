/*
 * Epix100aServer.hh
 *
 *  Created on: 2014.7.31
 *      Author: jackp
 */

#ifndef EPIX100ASERVER
#define EPIX100ASERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/epix100a/Epix100aManager.hh"
#include "pds/epix100a/Epix100aConfigurator.hh"
#include "pds/epix100a/Epix100aDestination.hh"
#include "pds/utility/Occurrence.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/service/GenericPool.hh"
#include "pds/evgr/EvrSyncCallback.hh"
#include "pds/evgr/EvrSyncRoutine.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
  class Epix100aServer;
  class Epix100aSyncSlave;
  class Task;
  class Allocation;
  class Routine;
  class EvrSyncCallback;
  class Epix100aServerSequence;
  class Epix100aServerCount;
  class EbCountSrv;
  class BldSequenceSrv;
}

class Pds::Epix100aServer
   : public EbServer
{
 public:
   Epix100aServer( const Src& client, unsigned configMask=0 );
   virtual ~Epix100aServer() {}
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtcTop.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtcTop; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtcTop.extent; }

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( char* payload, int flags );
   bool more() const;

   enum {DummySize = (1<<19)};

   void setEpix100a( int fd );

   unsigned configure(Epix100aConfigType*, bool forceConfig = false);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned numberOfElements(void) { return _elements; }
   unsigned flushInputQueue(int, bool printFlag = true);
   void     enable();
   void     disable();
   bool     g3sync() { return _g3sync; }
   void     g3sync(bool b) { _g3sync = b; }
   bool     ignoreFetch() { return _ignoreFetch; }
   void     ignoreFetch(bool b) { _ignoreFetch = b; }
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
   void     manager(Epix100aManager* m) { _mgr = m; }
   Epix100aManager* manager() { return _mgr; }
   void     process(char*);
   void     allocated(const Allocate& a) {_partition = a.allocation().partitionid(); }
   bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
   void     runTimeConfigName(char*);
   void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }
   void     fiberTriggering(bool b) { _fiberTriggering = b; }
   bool     fiberTriggering() { return _fiberTriggering; }
   bool     scopeEnabled() { return _scopeEnabled; }
   EpixSampler::ConfigV1*  samplerConfig() { return _samplerConfig; }
   const Xtc&      xtcConfig() { return _xtcConfig; }
   void     maintainLostRunTrigger(bool b) { _maintainLostRunTrigger = b; }
   Pds::Epix100a::Epix100aConfigurator* configurator() {return _cnfgrtr;}

 public:
   static Epix100aServer* instance() { return _instance; }

 private:
   static Epix100aServer*            _instance;
   static void instance(Epix100aServer* s) { _instance = s; }

 protected:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtcTop;
   Xtc                            _xtcEpix;
   Xtc                            _xtcSamplr;
   Xtc                            _xtcConfig;
   Pds::EpixSampler::ConfigV1*     _samplerConfig;
   unsigned                       _count;
   unsigned                       _fiducials;
   Pds::Epix100a::Epix100aConfigurator* _cnfgrtr;
   unsigned                       _elements;
   unsigned                       _elementsThisCount;
   unsigned                       _payloadSize;
   unsigned                       _configureResult;
   unsigned                       _debug;
   unsigned                       _offset;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   unsigned                       _ioIndex;
   Pds::Epix100a::Epix100aDestination     _d;
   char                           _runTimeConfigName[256];
   Epix100aManager*               _mgr;
   GenericPool*                   _occPool;
   unsigned                       _unconfiguredErrors;
   float                          _timeSinceLastException;
   unsigned                       _fetchesSinceLastException;
   char*                          _processorBuffer;
   unsigned*                      _scopeBuffer;
   Pds::Task*                     _task;
   Task*						              _sync_task;
   unsigned                       _partition;
   Epix100aSyncSlave*             _syncSlave;
   unsigned                       _countBase;
   unsigned                       _neScopeCount;
   unsigned*                      _dummy;
   int                            _myfd;
   unsigned                       _lastOpCode;
   bool                           _firstconfig;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _g3sync;
   bool                           _fiberTriggering;
   bool                           _ignoreFetch;
   bool                           _resetOnEveryConfig;
   bool                           _scopeEnabled;
   bool                           _scopeHasArrived;
   bool                           _maintainLostRunTrigger;
};

#include "pds/utility/EbCountSrv.hh"
class Pds::Epix100aServerCount
    :  public Epix100aServer,
       public EbCountSrv {
         public:
    Epix100aServerCount(const Src& client, unsigned configMask=0 )
         : Pds::Epix100aServer(client, configMask) {}
    ~Epix100aServerCount() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned count() const;
};

#include "pds/utility/BldSequenceSrv.hh"
class Pds::Epix100aServerSequence
    :  public Epix100aServer,
       public BldSequenceSrv {
         public :
    Epix100aServerSequence(const Src& client, unsigned configMask=0 )
         : Pds::Epix100aServer(client, configMask) {}
    ~Epix100aServerSequence() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned fiducials() const;
};

#endif
