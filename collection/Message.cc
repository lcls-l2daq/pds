#include "Message.hh"

using namespace Pds;

Message::Message(Type type, unsigned size) : 
  _type(type),
  _reply_to(),
  _size(size)
{}

Message::Message(Type type, const Ins& reply_to, unsigned size) : 
  _type(type),
  _reply_to(reply_to),
  _size(size)
{}

const Ins& Message::reply_to() const {return _reply_to;}
unsigned Message::size() const {return _size;}
unsigned Message::payload() const {return _size-sizeof(Message);}
void Message::reply_to(const Ins& ins) {_reply_to = ins;}
Message::Type Message::type() const {return _type;}
void Message::type(Type t) {_type = t;}

const char* Message::type_name() const 
{
  switch (_type) {
  case Ping:
    return "Ping";
  case Join:
    return "Join";
  case Resign:
    return "Resign";
  case Transition:
    return "Transition";
  default:
    return 0;
  }
}
