#ifndef CSPADSERVER
#define CSPADSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/service/Task.hh"
#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/cspad/CspadConfigurator.hh"
#include "pds/cspad/CspadDestination.hh"
#include "pds/cspad/CspadManager.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/cspad/Processor.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class Task;
   class CspadServer;
   class CspadServerSequence;
   class CspadServerCount;
   class EbCountSrv;
   class BldSequenceSrv;
}

class Pds::CspadServer
   : public EbServer
{
 public:
   CspadServer( const Src&, Pds::TypeId&, unsigned configMask=0 );
   virtual ~CspadServer();
    
   //  Eb interface
   void       dump ( int detail ) const {}
   bool       isValued( void ) const    { return true; }
   const Src& client( void ) const      { return _xtc.src; }

   //  EbSegment interface
   const Xtc& xtc( void ) const    { return _xtc; }
   unsigned   offset( void ) const;
   unsigned   length() const       { return _xtc.extent; }

   //  Server interface
   int pend( int flag = 0 ) { return -1; }
   int fetch( char* payload, int flags );
   bool more() const;

   void setCspad( int fd );

   unsigned configure(CsPadConfigType*);
   unsigned unconfigure(void);

   unsigned payloadSize(void)   { return _payloadSize; }
   unsigned numberOfQuads(void) { return _quads; }
   unsigned flushInputQueue(int, bool printFlag = true);
   void     enable();
   void     disable(bool flash = true);
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
   void     printState();
   void     manager(CspadManager* m) { _mgr = m; }
   CspadManager* manager() { return _mgr; }
   void     pgp(Pds::Pgp::Pgp* p) { _pgp = p; }
   Pds::Pgp::Pgp* pgp() { return _pgp; }
   void     sequenceServer(bool b) {_sequenceServer = b;}
   bool     sequenceServer() { return _sequenceServer;}

 public:
   static CspadServer* instance() { return _instance; }

 private:
   static CspadServer*            _instance;
   static void instance(CspadServer* s) { _instance = s; }

 protected:
   unsigned                       _debug;
   unsigned                       _count;
   unsigned                       _fiducials;
   unsigned                       _offset;

 private:
   enum     {sizeOfHisto=1000, sizeOfRHisto=1000, DummySize=(1<<19)};
   Xtc                            _xtc;
   Pds::CsPad::CspadConfigurator* _cnfgrtr;
   unsigned                       _quads;
   unsigned			                  _quadMask;
   unsigned                       _quadsThisCount;
   unsigned                       _payloadSize;
   unsigned                       _configMask;
   unsigned                       _configureResult;
   timespec                       _thisTime;
   timespec                       _lastTime;
   timespec                       _readTime1;
   timespec                       _readTime2;
   unsigned*                      _rHisto;
   unsigned*                      _histo;
   Pds::Task*                     _task;
   unsigned                       _ioIndex;
   Pds::CsPad::CspadDestination   _d;
   Pds::Pgp::Pgp*                 _pgp;
   unsigned*                      _dummy;
   char                           _runTimeConfigName[256];
   CspadManager*                  _mgr;
   GenericPool*                   _occPool;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _ignoreFetch;
   bool                           _sequenceServer;
};

#include "pds/utility/EbCountSrv.hh"
class Pds::CspadServerCount
    :  public CspadServer,
       public EbCountSrv {
         public:
    CspadServerCount( const Pds::Src& client, Pds::TypeId& myDataType, unsigned configMask )
         : Pds::CspadServer(client, myDataType, configMask) {}
    ~CspadServerCount() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned count() const;
};

#include "pds/utility/BldSequenceSrv.hh"
class Pds::CspadServerSequence
    :  public CspadServer,
       public BldSequenceSrv {
         public :
    CspadServerSequence( const Pds::Src& client, Pds::TypeId& myDataType, unsigned configMask)
         : Pds::CspadServer(client, myDataType, configMask) {}
    ~CspadServerSequence() {}
         public:
    //  Eb-key interface
    EbServerDeclare;
    unsigned fiducials() const;
};

#endif
