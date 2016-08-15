#include "pds/montag/Group.hh"

using namespace Pds::MonTag;

static unsigned _bits(unsigned maxelements)
{
  unsigned v=0;
  maxelements--;
  while(maxelements) {
    maxelements >>= 1;
    v++;
  }
  return v;
}

Group::Group(const std::string& name,
             unsigned           position,
             unsigned           maxmembers,
             unsigned           maxbuffers) :
  _name       (name),
  _position   (position),
  _max_members(maxmembers),
  _member_bits(_bits(maxmembers+1)),
  _buffer_bits(_bits(maxbuffers)),
  _no_request (0),
  _members    (0)
{
}

Tag Group::tag(int member) const
{
  return Tag(_position,_member_bits,_buffer_bits, member);
}

int Group::insert()
{
  for(unsigned i=1; i<=_max_members; i++)
    if ((_members & (1<<i))==0) {
      _members |= (1<<i);
      return i;
    }
  return -1;
}

void Group::remove(unsigned member)
{
  _members &= ~(1<<member);
}

void Group::push(unsigned member,
                 unsigned buffer)
{
  _requests.push_back(Tag::request(_position,
                                   _member_bits,
                                   _buffer_bits,
                                   member,
                                   buffer));
}

unsigned Group::pop()
{
  unsigned v;
  if (_requests.empty())
    v = _no_request;
  else {
    v = _requests.front();
    _requests.pop_front();
  }
  return v;
}
