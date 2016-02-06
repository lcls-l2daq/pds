#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "pds/ioc/IocConnection.hh"
#include "pds/ioc/IocControl.hh"
#include<list>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<strings.h>

//#define DBUG

using namespace Pds;

std::list<IocConnection*> IocConnection::_connections = std::list<IocConnection*>();

IocConnection::IocConnection(std::string host, uint32_t host_ip,
                             uint16_t port, IocControl *cntl) :
    _host(host),
    _host_ip(host_ip),
    _port(port),
    _cntl(cntl),
    _idx(1),
    _damage_req(0)
{
    _damage.clear();
    _damage.push_back(0);

    struct sockaddr_in serv_addr;
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(host_ip);
    serv_addr.sin_port = htons(port);
    /*
     * This will eventually timeout, but is it too long?
     */
    if (connect(_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        _cntl->_report_error("Cannot connect to host " + host);
        close(_sock);
        _sock = -1;
        return;
    }
    fcntl(_sock, F_SETFL, O_NONBLOCK);
}

IocConnection *IocConnection::get_connection(std::string host, uint32_t host_ip,
                                             uint16_t port, IocControl *cntl)
{
    for(std::list<IocConnection*>::iterator it=_connections.begin();
        it!=_connections.end(); it++)
        if ((*it)->_host_ip == host_ip && (*it)->_port == port)
            return *it;
    IocConnection *c = new IocConnection(host, host_ip, port, cntl);
    _connections.push_back(c);
    return c;
}

void IocConnection::transmit(const char *s)
{
#ifdef DBUG
    printf("IocConnection::transmit %s\n",s);
#endif
    int len = strlen(s), wlen;
    if (_sock < 0)
        return;
    if ((wlen = write(_sock, s, len)) != len) {
        /*
         * In principle, we might not have written everything.
         * In practice, we're writing short stuff, and the write totally failed.
         */
        perror("IocConnection write failed:");
        _cntl->_report_error("Connection to " + _host + " controls recorder has died.");
        close(_sock);
        _sock = -1;
    }
}

void IocConnection::transmit(std::string s)
{
#ifdef DBUG
    printf("IocConnection::transmit %s\n",s.c_str());
#endif
    int wlen;
    if (_sock < 0)
        return;
    if ((wlen = write(_sock, s.c_str(), s.length())) != (int) s.length()) {
        /*
         * In principle, we might not have written everything.
         * In practice, we're writing short stuff, and the write totally failed.
         */
        perror("IocConnection write failed:");
        _cntl->_report_error("Connection to " + _host + " controls recorder has died.");
        close(_sock);
        _sock = -1;
    }
}

void IocConnection::transmit_all(std::string s)
{
    for(std::list<IocConnection*>::iterator it=_connections.begin();
        it!=_connections.end(); it++)
        (*it)->transmit(s);
}

int IocConnection::check_all(void)
{
    for(std::list<IocConnection*>::iterator it=_connections.begin();
        it!=_connections.end(); it++)
        if ((*it)->_sock == -1)
            return 0;
    return 1;
}

int IocConnection::damage_status(int idx)
{
    char buf[1024], *s;
    unsigned int i;
    int len;

    if ((len = read(_sock, buf, sizeof(buf))) > 0 && !strncmp(buf, "dstat ", 6)) {
        buf[len] = 0;
        _damage_req = 0;
        for (i = 0, s = &buf[5]; s && i < _damage.size(); i++, s = index(s, ' ')) {
            _damage[i] = atoi(++s);
        }
    } else
        buf[0] = 0;
    if (!_damage_req) {
        _damage_req = 1;
        transmit("damage\n");
    }
    return _damage[idx];
}

