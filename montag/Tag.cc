#include "pds/montag/Tag.hh"

#include "pdsdata/xtc/Env.hh"

using namespace Pds::MonTag;

Tag::Tag(unsigned position,
         unsigned memberbits,
         unsigned bufferbits,
         unsigned member) :
  _position   (position),
  _member_bits(memberbits),
  _buffer_bits(bufferbits),
  _member     (member)
{
}

int Tag::buffer(const Env& env) const
{
  unsigned v = env.value();
  unsigned member = (v >> (_position+_buffer_bits))&((1<<_member_bits)-1);
  return (member == _member) ? (v>>_position)&((1<<_buffer_bits)-1) : -1;
}

unsigned Tag::request(unsigned position,
                      unsigned memberbits,
                      unsigned bufferbits,
                      unsigned member,
                      unsigned buffer)
{
  const unsigned member_mask = (1<<memberbits)-1;
  const unsigned buffer_mask = (1<<bufferbits)-1;

  unsigned v = member&member_mask;
  v <<= bufferbits;
  v |=  buffer&buffer_mask;
  v <<= position;
  return v;
}
