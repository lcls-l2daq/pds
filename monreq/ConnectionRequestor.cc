//ConnectionRequestor.cc

#include "pds/monreq/ConnectionRequestor.hh"
#include "pds/monreq/ConnInfo.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"

#include <stdio.h>
#include <stdlib.h>

using namespace Pds ;
using namespace Pds::MonReq ;

    
      //
      //  Open a UDP socket on "interface"
      //  Will request connections to "nodes" on TCP "interface","port"
      //
      ConnectionRequestor::ConnectionRequestor(int interface, unsigned nodes, Ins mcast) : _mcast(mcast) {

		_listen_socket = ::socket(AF_INET,SOCK_STREAM,0);
        	if (_listen_socket == -1) {
        	perror("Could not create socket");
		exit (1);
        	}
		printf("tcp socket created\n");


		Sockaddr listen_server(mcast.portId());


		int y=1;
        	if(setsockopt(_listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1) {
		perror("set reuseaddr");
		exit (1);
       		}
	
		if (bind(_listen_socket, listen_server.name(), listen_server.sizeofName()) < 0) {
		perror("tcp bind error: \n");
		exit (1);
		}
		printf("tcp bind successful\n");

		_info = ConnInfo(interface, listen_server.get().portId(), nodes);
	
        	listen(_listen_socket, 5);
	

    		_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        	if (_socket == -1) {
        	puts("Could not create udp socket");
		exit (1);
        	}
		puts("Udp socket created");

		//printf("ip: %i, port: %i, node: %i\n", _iip, _psa.get().portId(), _node);
		

 		Sockaddr uv(interface);
		if (bind(_socket, uv.name(), uv.sizeofName()) < 0) {
		perror("bind");
		exit (1);
		}

		y=1;
  		if(setsockopt(_socket, SOL_SOCKET, SO_BROADCAST, (char*)&y, sizeof(y)) == -1) {
   		 perror("set broadcast");
   		 exit (1);
  		}

	}

      int ConnectionRequestor::socket() {
      return _listen_socket;
      }

      //
      //  Receive requested connections
      //
      int ConnectionRequestor::receiveConnection() {
		int i;
		Sockaddr listen_client;
		socklen_t sa;
		if ( (i = accept(_listen_socket, listen_client.name(), &sa)) == -1){
			perror("tcp accept failed: \n");
			return i;
		}
			printf("tcp accept\n");
			return i;
	
	}
      //
      //  Send out request for connections from "nodes"
      //
      int ConnectionRequestor::requestConnections() {
	
		if (sendto(_socket, &_info, sizeof(_info), 0, _mcast.name(), _mcast.sizeofName()) == -1) {
		perror("sending ip, port, node");
		return -1;
		}
		return 0;

	}



