#ifndef PDSCOLLECTION_SERVERMANAGER_HH
#define PDSCOLLECTION_SERVERMANAGER_HH

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

class CollectionServerManager: public ServerManager {
public:
  CollectionServerManager(const Ins& mcast,
                          Level::Type level,
                          unsigned platform,
                          unsigned maxpayload,
                          unsigned timeout,
                          Arp* arp);
  virtual ~CollectionServerManager();

public:
  const Node& header() const;

public:
  void start();
  void cancel(); // Synchronous, to be used only outside our task

public:
  void arpadd(const Node& hdr);

public:
  void ucast(Message& msg, const Ins& dst);
  void mcast(Message& msg);

public:
  virtual void message(const Node& hdr, const Message& msg);

private:
  // Implements Server from ServerManager
  virtual char* payload();
  virtual int handleError(int value);

  // Implements ServerManager
  virtual int processIo(Server* srv);

protected:
  void _send(const Message& msg, const Ins& dst);
  void _send(const Message& msg, const Ins& dst, const Node& hdr);

protected:
  void _connected();
  void _disconnected();

private:
  Task* _task;
  SelectDriver _driver;

protected:
  Node _header;
  RouteTable _table;
  Client _client;
  char* _payload;
  Arp* _arp;
  Ins _mcast;
  CollectionServer* _mcastServer;
  CollectionServer* _ucastServer;
};

}
#endif
