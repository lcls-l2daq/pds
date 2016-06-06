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
  ///  DAQ hierarchy level
  Level::Type     level    () const;
  ///  DAQ platform number
  unsigned        platform () const;
  ///  Readout group (for Segment levels)
  unsigned        group    () const;
  ///  Physical address (for Segment levels)
  unsigned        paddr    () const;
  ///  Is a triggered Segment level
  bool            triggered() const;
  ///  Which EVR module provides the trigger
  unsigned        evr_module() const;
  ///  Which EVR output trigger channel 
  unsigned        evr_channel() const;
  ///  Prune before recording
  bool            transient() const;
  ///  Process ID
  int             pid      () const;
  ///  User ID
  int             uid      () const;
  ///  IP address
  int             ip       () const;
  ///  Ethernet MAC address
  const Ether&    ether    () const;
  ///  
  const ProcInfo& procInfo () const;
  int operator == (const Node& rhs) const;

public:
  static int user_name(int uid, char* buf, int bufsiz);
  static int ip_name  (int ip, char* buf, int bufsiz);

public:
  void fixup       (int ip, const Ether& ether);
  void setGroup    (unsigned group);
  void setPaddr    (unsigned paddr);
  void setTransient(bool);
  void setTrigger  (unsigned module,unsigned channel);
private:
  uint16_t  _platform;
  uint16_t  _group;    // segment level only: each group has different readout rates
  uint32_t  _paddr;    // segment level only: fast control physical address 
  int32_t   _uid;
  ProcInfo  _procInfo;
  Ether     _ether;
};

}
#endif
