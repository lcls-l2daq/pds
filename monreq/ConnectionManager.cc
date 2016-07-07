//ConnectionManager.cc

#include "pds/monreq/ConnectionManager.hh"
#include "pds/monreq/ConnInfo.hh"
#include "pds/service/Ins.hh"
#include "pds/service/Sockaddr.hh"

#include <stdio.h>
#include <stdlib.h>


using namespace Pds;
using namespace Pds::MonReq;

      //
      //  Open a UDP socket on "interface" and join multicast group "mcast"
      //

	ConnectionManager::ConnectionManager(int interface, const Ins& mcast, int id, std::vector<ServerConnection>& servers) : _id(id), _servers(servers) {
		
		_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        	if (_socket == -1) {
        	perror("Could not create udp socket");
		     }
		puts("UDP SOCKET CREATED");
		
		Sockaddr psa(mcast); 
        	if (bind(_socket,(struct sockaddr *)psa.name(), psa.sizeofName()) < 0) {
	 	perror("udp bind error");
	  	exit (1);
     	 	}
		puts("UDP BIND SUCCESS");

		struct ip_mreq m;
		m.imr_multiaddr.s_addr= htonl(mcast.address()); 
     		m.imr_interface.s_addr=htonl(interface); 

     		if (setsockopt(_socket, IPPROTO_IP,IP_ADD_MEMBERSHIP, &m, sizeof(m)) < 0) {
	  	perror("setsockopt");
	  	exit (1);
          	}
		puts("SETSOCKOPT SUCCESS");

	}

	int ConnectionManager::socket() const {
	return _socket;
	}


      //
      //  Receive multicast connection request and return connected socket descriptor
      //
	int ConnectionManager::receiveConnection() {

			ConnInfo c;
			if (recv(_socket, &c, sizeof(c), 0) < 0) {
			perror("Recvfrom error:");
			return -1;
			}
		int node = c.node_num;
		//printf("Node vs 1<<id: %i, %i\n", node, (1<<_id));
		
		{in_addr inaddr;
		inaddr.s_addr = htonl(c.ip_add);
     		char* tcp_ip = inet_ntoa(inaddr);
                int port = c.port_num;
		//printf( "ip: %s, port: %i, node: %i\n", tcp_ip, port, node);}

		if(node&(1<<_id)) {



			for (unsigned i=0; i< _servers.size(); i++) {
			//printf("address %i %i\n", _servers[i].address(), i);
				if (_servers[i].address() == c.ip_add ) {
				return -1;
				}
			}


			struct sockaddr_in tcp_server;
        		int tcp_socket_info = ::socket(AF_INET, SOCK_STREAM, 0);
       		 	if (tcp_socket_info == -1) {
        		printf("Could not create socket");
			return -1;
        		}
        		printf("TCP socket created\n");

        		tcp_server.sin_addr.s_addr = htonl(c.ip_add);
        		tcp_server.sin_family = AF_INET;
        		tcp_server.sin_port = htons(c.port_num);

			

        		if (::connect(tcp_socket_info, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) < 0) {
        		perror("TCP Connection error");
			return -1;
        		}
        		puts("TCP Connected");
			return tcp_socket_info;
			
		
		}
		return -1;
	}

	
	};	

