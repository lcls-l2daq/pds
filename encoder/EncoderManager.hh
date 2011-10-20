#ifndef ENCODERMANAGER_HH
#define ENCODERMANAGER_HH

#include "pds/utility/Appliance.hh"
#include "pds/client/Fsm.hh"
#include "EncoderOccurrence.hh"

namespace Pds {
   class EncoderServer;
   class EncoderManager;
   class EncoderOccurrence;
   class CfgClientNfs;
}

class Pds::EncoderManager {
 public:
   EncoderManager( EncoderServer* server,
                   CfgClientNfs* cfg );
   Appliance& appliance( void ) { return _fsm; }

 private:
   Fsm& _fsm;
   static const char* _calibPath;
   EncoderOccurrence* _occSend;
};

#endif
