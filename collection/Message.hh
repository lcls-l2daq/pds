#ifndef PDSCOLLECTION_MESSAGE_HH
#define PDSCOLLECTION_MESSAGE_HH

#include "pds/service/Ins.hh"

namespace Pds {

class Message {
public:
  enum Type {
    Ping, 
    Join, 
    Resign, 
    Transition,
    Query,
    Occurrence,
    Alias
  };

  Message(Type type, unsigned size=sizeof(Message));
  Message(Type type, const Ins& reply_to, unsigned size=sizeof(Message));

  const Ins& reply_to() const;
  unsigned size() const;
  unsigned payload() const;

public:
  void reply_to(const Ins& ins);
  Type type() const;
  void type(Type t);
  const char* type_name() const;

protected:
  Type _type;
  Ins _reply_to;
  unsigned _size;
};

}
#endif
