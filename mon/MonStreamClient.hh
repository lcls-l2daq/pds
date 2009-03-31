#ifndef Pds_MonStreamClient_hh
#define Pds_MonStreamClient_hh

#include "pds/mon/MonClient.hh"

namespace Pds {

  class MonStreamSocket;

  class MonStreamClient : public MonClient {
  public:
    MonStreamClient(MonConsumerClient&,
		    MonCds*,
		    MonStreamSocket* socket,
		    const Src&);
    ~MonStreamClient();
  private:
    MonStreamSocket* _ssocket;
  };

};

#endif
