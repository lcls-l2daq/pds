#ifndef PDSCOLLECTION_MANAGER_HH
#define PDSCOLLECTION_MANAGER_HH

#include "pds/service/ServerManager.hh"
#include "pds/service/SelectDriver.hh"
#include "pds/service/Client.hh"

#include "Node.hh"
#include "RouteTable.hh"

namespace Pds {

class Arp;
class Task;
class CollectionServer;
class Message;

class CollectionManager: public ServerManager {
public:
  CollectionManager(Level::Type level,
                    unsigned partition,
                    unsigned maxpayload,
                    unsigned timeout,
                    Arp* arp=0);
  CollectionManager(unsigned maxpayload,
                    Arp* arp=0);
  virtual ~CollectionManager();

public:
  const Node& header() const;

public:
  void connect();
  void disconnect();
  void cancel(); // Synchronous, to be used only outside our task

public:
  void mcast(Message& msg);
  void ucast(Message& msg, const Ins& dst);
  void arpadd(const Node& hdr);

public:
  virtual void message(const Node& hdr, const Message& msg) = 0;
  virtual void connected(const Node& hdr,const Message& msg) = 0;
  virtual void timedout() = 0;
  virtual void disconnected() = 0;

private:
  // Implements Server from ServerManager
  virtual char* payload();
  virtual int commit(char* datagram, char* load, int size, const Ins& src);
  virtual int handleError(int value);

  // Implements ServerManager
  virtual int processIo(Server* srv);
  virtual int processTmo();

  void _send(const Message& msg, const Ins& dst);
  void _send(const Message& msg, const Ins& dst, const Node& hdr);
  void _build_servers();
  void _destroy_servers();

private:
  Task* _task;
  SelectDriver _driver;
  Node _header;
  RouteTable _table;
  Client _client;
  char* _payload;
  Ins _mcast;
  CollectionServer* _mcastServer;
  CollectionServer* _ucastServer;
  Arp* _arp;
};

}
#endif
