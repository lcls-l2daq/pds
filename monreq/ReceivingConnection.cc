//ReceivingConnection.cc

#include "pds/monreq/ReceivingConnection.hh"
#include "pds/monreq/ConnInfo.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <stdio.h>
#include <stdlib.h>

#include "pds/vmon/VmonEb.hh"
#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonDescScalar.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"

using namespace Pds;
using namespace Pds::MonReq;
   
      //
      //  This is the connection for receiving datagrams on TCP "socket"
      //
      ReceivingConnection::ReceivingConnection(int socket, MonGroup* group): _socket(socket), _group(group), _numberRequested(0), _numberReceived(0) {

	//
	//Plot setups Number Requested/Received
	//
	MonDescScalar numberRequested_count("Number Requested");
        _numberRequested_count = new MonEntryScalar(numberRequested_count);
        _group->add(_numberRequested_count);

	MonDescScalar numberReceived_count("Number Received");
        _numberReceived_count = new MonEntryScalar(numberReceived_count);
        _group->add(_numberReceived_count);

	}

      //
      //  Return the socket number for setting up poll
      //
      int ReceivingConnection::socket() const {
	return _socket;
	}

      //
      //  Make a new request
      //
      int ReceivingConnection::request() {


		_numberRequested= _numberRequested +1; 
		
		//update plot value
		_numberRequested_count->setvalue(_numberRequested);		

		if (send(_socket, (char*)&_numberRequested , sizeof(_numberRequested) , 0) < 0) {
		perror("SEND TO REQUEST NUMBER: ");
		return -1;
		} 
		timespec tv;
		clock_gettime(CLOCK_REALTIME, &tv);
			if(_numberReceived % 30 == 0) {
			//printf("REQUEST_NUMBER, TIME: %i, %ld\n", _numberRequested, tv.tv_sec);
			}
		return 0;
      }

      //
      //  Receive a new event
      //
      char* ReceivingConnection::receive() {

		Pds::Dgram datagram;
                         int nb = recv(_socket, &datagram, sizeof(datagram), 0) ;
		
				if ( datagram.seq.isEvent() ) {
				_numberReceived= _numberReceived + 1;
				//printf("RECEIVED NUMBER: %i\n", _numberReceived);
				
				//update plot value
				_numberReceived_count->setvalue(_numberReceived);
				_numberReceived_count->time(datagram.seq.clock());
				_numberRequested_count->time(datagram.seq.clock());
				
					if(_numberReceived % 120 == 0) {
					//printf("RECEIVED, TIME: %i %lf \n", _numberReceived, datagram.seq.clock().asDouble());
					}
				}


                         if (nb==0) return 0;
                         	if  (nb == -1) {
				perror("recv");
				return 0;
				}
        		 //printf("%i [%d]\n", nb, datagram.xtc.sizeofPayload());

         		char* p = new char[sizeof(datagram)+datagram.xtc.sizeofPayload()];
         		memcpy(p, &datagram, sizeof(datagram));
	 		char* q = p+sizeof(datagram);
         		int remaining = datagram.xtc.sizeofPayload();
         			do {
           			nb = recv(_socket, q, remaining, 0);
           			remaining -= nb;
           			q += nb;
         			} while (remaining>0);
			return p;			
	
         
	}

	//
	// Calulated ratio to determine 'best' to send requests to
	//
	int ReceivingConnection::ratio() const {

	int r = _numberRequested - _numberReceived;
	return r;

	}


	





