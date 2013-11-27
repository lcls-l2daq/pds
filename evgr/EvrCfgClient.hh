#ifndef Pds_EvrCfgClient_hh
#define Pds_EvrCfgClient_hh

#include "pds/config/CfgClientNfs.hh"

#include <list>

namespace Pds {

  class EvrCfgClient : public CfgClientNfs {
  public:
    EvrCfgClient( const Src& src, char* eclist );
    ~EvrCfgClient();

    int fetch(const Transition& tr, 
	      const TypeId&     id, 
	      void*             dst,
	      unsigned          maxSize=0x100000);

  private:
    char*         _buffer;
    std::list<unsigned> _default_codes;
  };

};

#endif

    
