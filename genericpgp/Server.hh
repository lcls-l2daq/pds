/*
 * Server.hh
 *
 */

#ifndef GenericPgpServer_hh
#define GenericPgpServer_hh

#include "pds/genericpgp/PeriodMonitor.hh"
#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/genericpgp/Configurator.hh"
#include "pds/genericpgp/Destination.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
  class Appliance;
  class GenericPool;
  class Task;

  namespace GenericPgp {
    class Server
      : public EbServer,
        public EbCountSrv {
    public:
      Server( const Src& client, unsigned configMask=0 );
      virtual ~Server() {}
    
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
      void setFd( int fd );

      unsigned configure(GenericPgpConfigType*, bool forceConfig = false);
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
      void     appliance(Appliance& m) { _app = &m; }
      void     process(char*);
      void     allocated();
      bool     resetOnEveryConfig() { return _resetOnEveryConfig; }
      void     runTimeConfigName(char*);
      void     resetOnEveryConfig(bool r) { _resetOnEveryConfig = r; }

    public:
      static Server* instance() { return _instance; }
      //   void                laneTest();

    private:
      static Server*            _instance;
      static void instance(Server* s) { _instance = s; }

    private:
      enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
      Xtc                            _xtc;
      Configurator*                  _cnfgrtr;
      unsigned                       _elements;
      unsigned                       _count;
      unsigned                       _elementsThisCount;
      unsigned                       _payloadSize;
      unsigned                       _configureResult;
      unsigned                       _debug;
      unsigned                       _offset;
      Task*                          _task;
      unsigned                       _ioIndex;
      Destination                    _d;
      char                           _runTimeConfigName[256];
      Appliance*                     _app;
      GenericPool*                   _occPool;
      unsigned                       _unconfiguredErrors;
      std::vector<Xtc>               _payload_xtc;
      std::vector<char*>             _payload_buffer;
      unsigned                       _payload_mask;
      bool                           _configured;
      bool                           _ignoreFetch;
      bool                           _resetOnEveryConfig;
      PeriodMonitor                  _monitor;
    };
  };
};

#endif
