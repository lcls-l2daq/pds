//ServerConnection.cc

#include "pds/monreq/ServerConnection.hh"
#include "pds/monreq/ConnInfo.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"
#include "pds/xtc/Datagram.hh"

#include <stdio.h>
#include <stdlib.h>

using namespace Pds;
using namespace Pds::MonReq;

//
//  Track the connection to a requestor on TCP "socket"
//
ServerConnection::ServerConnection(int socket): _socket(socket), _numberRequested(0), _numberSent(0) {
  Sockaddr k;	
  socklen_t ka=k.sizeofName();
  if( getpeername( _socket, k.name(), &ka) <0 ) {
    perror("getpeername");
  }
	

  _address = k.get().address();  
}


//
//  Return the socket number for setting up poll
//
int ServerConnection::socket() const {
  return _socket;
}


int ServerConnection::address() const {
  return _address;
}

//
//  Receive a new request
//
int ServerConnection::recv() {

  if(::recv(_socket, (char *)&_numberRequested, sizeof(_numberRequested), 0) < 0) {
    perror("receive number requested");
    return -1;
  }
  //printf("NUMBER OF REQUESTS RECEIVED: %i\n", _numberRequested);
  return 0;
}

//
//  Fulfill an outstanding request, if necessary
//
int ServerConnection::send(const Datagram& dg) {


  //if we have any requests
  if (dg.seq.service()==TransitionId::L1Accept) { 

    if(_numberSent < _numberRequested) {


      // send an event
      int ca = ::send(_socket, &dg, sizeof(dg)+ dg.xtc.sizeofPayload(), MSG_NOSIGNAL );
      //printf("Datagram that is sending: %i\n", ca);

      //checking if sending correctly
      //printf("sizeofPayload: [%d]\n", dg.xtc.sizeofPayload());
		             
      _numberSent = _numberSent +1;
      //printf("REQUESTS FULLFILLED: %i\n", _numberSent);
    }
  }
  else {
    int ca = ::send(_socket, &dg, sizeof(Dgram)+ dg.xtc.sizeofPayload(), MSG_NOSIGNAL );
    //printf("%i\n", ca);
  }
			
  return 0;         
}






		
