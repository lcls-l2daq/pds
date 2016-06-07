//ConnInfo.hh

#ifndef ConnInfo_hh
#define ConnInfo_hh

namespace Pds {

	class ConnInfo
		{
		public:
		ConnInfo(){
		}		

		ConnInfo(int i, int p, int n) { 

			ip_add= i;
			port_num= p;
			node_num = n;
			}

		int ip_add;
		int port_num;
		int node_num;
		};

};

#endif
