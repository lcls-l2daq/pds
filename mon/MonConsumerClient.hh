#ifndef Pds_MonCONSUMER_HH
#define Pds_MonCONSUMER_HH

namespace Pds {

  class MonClient;

  class MonConsumerClient {
  public:
    virtual ~MonConsumerClient() {}
    enum Type {ConnectError,
	       DisconnectError,
	       Connected, 
	       Disconnected, 
	       Enabled, 
	       Disabled, 
	       Description, 
	       Payload};
    virtual void process(MonClient& client, Type type, int result=0) = 0;
  };
};

#endif
