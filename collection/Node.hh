#ifndef PDSNODE_HH
#define PDSNODE_HH

#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "Ether.hh"

namespace Pds {

class Node {
public:
  Node();
  Node(const Node& rhs);
  Node(Level::Type level, unsigned platform);

public:
  Level::Type level() const;
  unsigned platform() const;
  int pid() const;
  int uid() const;
  int ip() const;
  const Ether& ether() const;
  const ProcInfo& procInfo() const;
  int operator == (const Node& rhs) const;

public:
  static int user_name(int uid, char* buf, int bufsiz);
  static int ip_name(int ip, char* buf, int bufsiz);

public:
  void fixup(int ip, const Ether& ether);

private:
  unsigned _platform;
  int _uid;
  ProcInfo _procInfo;
  Ether _ether;
};

}
#endif
