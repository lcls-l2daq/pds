#ifndef CSPAD2x2SERVER
#define CSPAD2x2SERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/CsPad2x2ConfigType.hh"
#include "pds/service/Task.hh"
#include "pdsdata/cspad2x2/ElementHeader.hh"
#include "pds/cspad2x2/Cspad2x2Configurator.hh"
#include "pds/cspad2x2/Cspad2x2Destination.hh"
#include "pds/cspad2x2/Processor.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class Cspad2x2Server;
   class Task;
}

class Pds::Cspad2x2Server
   : public EbServer,
     public EbCountSrv
{
 public:
   Cspad2x2Server( const Src&, Pds::TypeId&, unsigned configMask=0 );
   virtual ~Cspad2x2Server() {}
    
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
   int fetch( ZcpFragment& , int flags ) { return 0; }
   int fetch( char* payload, int flags );
   bool more() const;

   unsigned count() const;
   void setCspad2x2( int fd );

   unsigned configure(CsPad2x2ConfigType*);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned flushInputQueue(int);
   unsigned enable();
   unsigned disable();
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
   void     ignoreFetch(bool f) { _ignoreFetch = f; }
   void     runTimeConfigName(char*);
   void     runTrigFactor(unsigned f) { _runTrigFactor = f; }
   unsigned runTrigFactor() { return _runTrigFactor; }

 public:
   static Cspad2x2Server* instance() { return _instance; }

 private:
   static Cspad2x2Server*            _instance;
   static void instance(Cspad2x2Server* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, DummySize=(1<<19)};
   Xtc                            _xtc;
   Pds::CsPad2x2::Cspad2x2Configurator* _cnfgrtr;
   unsigned                       _count;
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
   Pds::CsPad2x2::Cspad2x2Destination   _d;
   Pds::Pgp::Pgp*                 _pgp;
   unsigned*                      _dummy;
   char                           _runTimeConfigName[256];
   unsigned                       _runTrigFactor;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _ignoreFetch;
};

#endif
