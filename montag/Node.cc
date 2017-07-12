#include "pds/montag/Node.hh"
#include "pds/montag/Group.hh"

using namespace Pds::MonTag;

Node::Node(int        socket,
           int        member,
           Group&     group) :
        _socket(socket),
        _member(member),
        _group (group) 
{
}

Node::~Node() 
{
  _group.remove(_member); 
}
