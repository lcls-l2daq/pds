#ifndef PDSNODE_HH
#define PDSNODE_HH

#include "Level.hh"
#include "Ether.hh"

namespace Pds {

class Node {
public:
  Node();
  Node(const Node& rhs);
  Node(Level::Type level, unsigned partition);

public:
  Level::Type level() const;
  unsigned partition() const;
  int pid() const;
  int uid() const;
  int ip() const;
  const Ether& ether() const;
  int operator == (const Node& rhs) const;

public:
  static int user_name(int uid, char* buf, int bufsiz);
  static int ip_name(int ip, char* buf, int bufsiz);

private:
  friend class CollectionManager;
  void fixup(int ip, const Ether& ether);

private:
  Level::Type _level;
  unsigned _partition;
  int _pid;
  int _uid;
  int _ip;
  Ether _ether;
};

}
#endif
