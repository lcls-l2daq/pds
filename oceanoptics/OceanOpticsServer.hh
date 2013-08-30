#ifndef OCEANOPTICS_SERVER_HH
#define OCEANOPTICS_SERVER_HH

#include "pds/utility/EbServer.hh"
#include "pds/utility/EbCountSrv.hh"
#include "pds/utility/EbEventKey.hh"
#include "pds/config/OceanOpticsConfigType.hh"

#include <stdexcept>

namespace Pds
{

class OceanOpticsServer
   : public EbServer,
     public EbCountSrv
{
 public:
   OceanOpticsServer( const Src& client, int iDevice, int iDebugLevel );
   virtual ~OceanOpticsServer();
    
   //  Eb interface
   virtual void       dump    (int detail)  const {}
   virtual bool       isValued()            const { return true; }
   virtual const Src& client  ()            const { return _xtc.src; }

   //  EbSegment interface
   virtual const Xtc& xtc   () const { return _xtc; }
   virtual unsigned   offset() const { return sizeof(Xtc); }
   virtual unsigned   length() const { return _xtc.extent; }

   //  Eb-key interface
   EbServerDeclare;

   //  Server interface
   virtual int pend (int flag = 0)             { return -1; }
   virtual int fetch(ZcpFragment& , int flags) { return 0; }
   virtual int fetch(char* payload, int flags);

   // EbCountSrv interface
   virtual unsigned count() const;
   
   bool     devicePresent () const;

   unsigned configure     (OceanOpticsConfigType& config);
   unsigned unconfigure   ();
   
   enum 
   {
     ERR_DEVICE_NOT_PRESENT = 100,
     ERR_LIBOOPT_FAIL       = 101,
     ERR_OPEN_FAIL          = 102,
     ERR_READ_FAIL          = 103,
     ERR_OTHER              = 104          
   };

 private:   
   Xtc          _xtc;
   uint64_t     _count;   
   int          _iDebugLevel;   
}; //class OceanOpticsServer

class OceanOpticsServerException : public std::runtime_error
{
public:
  explicit OceanOpticsServerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} //namespace Pds

#endif //#ifndef OCEANOPTICS_SERVER_HH
