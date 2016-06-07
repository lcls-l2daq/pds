#ifndef Pds_MonReq_ReceivingConnection_hh
#define Pds_MonReq_ReceivingConnection_hh

#include "pds/vmon/VmonEb.hh"
#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonDescScalar.hh"

namespace Pds {

  namespace MonReq {

    class ReceivingConnection {
    public:
      //
      //  This is the connection for receiving datagrams on TCP "socket"
      //
      ReceivingConnection(int socket, MonGroup* group);

    public:
      //
      //  Return the socket number for setting up poll
      //
      int socket() const;

      //
      //  Make a new request
      //
      int request();

      //
      //  Receive a new event
      //
      char* receive();

      //
      //  Determines ratio of requested:fullfilled events
      //
      int ratio() const;	

      //
      //  Sends group to make plots
      //   
      void send_group(MonGroup* g);


    private:
      int _socket;   // Descriptor for connected TCP socket
      int _numberRequested;
      int _numberReceived;
      MonGroup* _group;
	MonEntryScalar*   _numberRequested_count;
	MonEntryScalar*   _numberReceived_count;
    };

  }
}
#endif
