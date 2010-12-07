#ifndef Pds_EvrCfgClient_hh
#define Pds_EvrCfgClient_hh

#include "pds/config/CfgClientNfs.hh"

namespace Pds {

  class EvrCfgClient {
  public:
    EvrCfgClient( CfgClientNfs& c );
    ~EvrCfgClient();

    const Src& src() const;

    void initialize(const Allocation& a);

    int fetch(const Transition& tr, 
	      const TypeId&     id, 
	      void*             dst,
	      unsigned          maxSize=0x100000);

  private:
    CfgClientNfs& _c;
    char*         _buffer;
  };

};

#endif

    
