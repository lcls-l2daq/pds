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
  Node(Level::Type level, uint16_t platform);

public:
  Level::Type     level() const;
  uint16_t        platform() const;
  uint16_t        group() const;
  int             pid() const;
  int             uid() const;
  int             ip() const;
  const Ether&    ether() const;
  const ProcInfo& procInfo() const;
  int operator == (const Node& rhs) const;

public:
  static int user_name(int uid, char* buf, int bufsiz);
  static int ip_name(int ip, char* buf, int bufsiz);

public:
  void fixup(int ip, const Ether& ether);
  void setGroup(uint16_t group);

private:
  uint16_t  _platform;
  uint16_t  _group;    // segment level only: each group has different readout rates
  int32_t   _uid;
  ProcInfo  _procInfo;
  Ether     _ether;
};

}
#endif
