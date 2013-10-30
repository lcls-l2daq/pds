#ifndef Pds_IocConnection_hh
#define Pds_IocConnection_hh
#include<string>
#include<list>

namespace Pds {
  class IocControl;

  class IocConnection {
  public:
    IocConnection(std::string host, uint32_t host_ip, uint16_t port, IocControl *cntl);
    ~IocConnection() { close(_sock); _sock = -1; };
    static IocConnection *get_connection(std::string host, uint32_t host_ip,
                                         uint16_t port, IocControl *cntl);
    void transmit(char *s);
    void transmit(std::string s);
    static void transmit_all(std::string s);
    static void clear_all(void) { _connections.clear(); };
    static std::list<IocConnection*> _connections;
    std::string host() { return _host; }
  private:
    int _sock;
    std::string _host;
    uint32_t _host_ip;
    uint16_t _port;
    IocControl *_cntl;
  };
};

#endif
