#ifndef FEXAMPSERVER
#define FEXAMPSERVER

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/PhasicsConfigType.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pds/phasics/PhasicsConfigurator.hh"
#include "pdsdata/xtc/Xtc.hh"
#include <fcntl.h>
#include <time.h>

namespace Pds
{
   class PhasicsServer;
   class Task;
   class Routine;
   class PhasicsReceiver;
   class PhasicsImageHisto;
}

class Pds::PhasicsImageHisto {
  public:
    PhasicsImageHisto(unsigned char* p) : next(0), ptr(p), count(1) {};
    ~PhasicsImageHisto() {};

  public:
    void print();
    bool operator==(PhasicsImageHisto&);
    bool isNew(PhasicsImageHisto&);

  public:
    Pds::PhasicsImageHisto*  next;
    unsigned char*           ptr;
    unsigned                 count;
};

class Pds::PhasicsReceiver : public Pds::Routine {
  public:
  PhasicsReceiver(dc1394camera_t* c, int* i, int* o) :
    _camera(c), in(i), out(o), runFlag(true)  {}
  virtual ~PhasicsReceiver() {}

  public:
  void routine(void);
  void printError(char*);
  void camera(dc1394camera_t* c) { _camera = c;     }
  void die()                     { runFlag = false; }
  static void resetCount()       { count = 0;       }
  static void resetFirst(bool f)       { first = f;    }
  void waitForNotFirst();
  static unsigned             count;
  static bool                first;
  dc1394camera_t*            _camera;
  int*                       in;
  int*                       out;
  dc1394video_frame_t*       frame;
  dc1394error_t              _err;
  bool                       runFlag;
};

class Pds::PhasicsServer
   : public EbServer,
     public EbCountSrv
{
 public:
   PhasicsServer( const Src& client );
   virtual ~PhasicsServer() {}
    
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

   unsigned        count() const;
   void            setPhasics( int fd );

   unsigned        configure(PhasicsConfigType*);
   unsigned        unconfigure(void);

   unsigned        payloadSize(void)   { return _payloadSize; }
   void            enable();
   void            disable();
   void            debug(unsigned d) { _debug = d; }
   unsigned        debug() { return _debug; }
   void            resetCount() {_count = 0; }
   unsigned        myCount() { return _count; }
   void            printHisto(bool);
   void            printError(char*);
   void            process(void);
   void            allocated();
   void            camera(dc1394camera_t* c) { _camera = c; }
   dc1394camera_t* camera() { return _camera; }

   bool            dropTheFirst() { return _dropTheFirst; }
   void            dropTheFirst(bool d)  { _dropTheFirst = d; }
   void            printHisto()   { if (_iHisto) {printf("\t----IHisto---\n"); _iHisto->print(); }}

 public:
   static PhasicsServer* instance() { return _instance; }

 private:
   static PhasicsServer*            _instance;
   static void instance(PhasicsServer* s) { _instance = s; }

 private:
   enum     {sizeOfHisto=1000, ElementsPerSegmentLevel=1};
   Xtc                            _xtc;
   Pds::Phasics::PhasicsConfigurator* _cnfgrtr;
   unsigned                       _count;
   unsigned                       _payloadSize;
   unsigned                       _imageSize;
   unsigned                       _configureResult;
   int                            _s2rFd[2];
   int                            _r2sFd[2];
   dc1394camera_t*                _camera;
   dc1394video_frame_t*           _frame;
   dc1394error_t                  _err;
   unsigned                       _debug;
   timespec                       _thisTime;
   timespec                       _lastTime;
   unsigned*                      _histo;
   Task*                          _task;
   PhasicsReceiver*               _receiver;
   PhasicsImageHisto*             _iHisto;
   unsigned                       _iHistoEntries;
   unsigned                       _iHistoEntriesMax;
   unsigned                       _unconfiguredErrors;
   bool                           _configured;
   bool                           _firstFetch;
   bool                           _enabled;
   bool                           _dropTheFirst;
};

#endif
