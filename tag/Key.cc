#include "pds/tag/Key.hh"

using namespace Pds::Tag;

Key::Key(Option) :
  _platform(0), _member(0), _group(-1)
{
}

Key::Key(unsigned platform,
         int      group,
         unsigned member) :
  _platform(platform),
  _member  (member),
  _group   (group)
{
}

bool     Key::valid   () const { return _group>=0; }

unsigned Key::platform() const { return _platform; }

int      Key::group() const { return _group; }

unsigned Key::member() const { return _member; }

uint32_t Key::mask() const { return 0xff << (16+8*_group); }

uint32_t Key::tag(unsigned buffer) const
{
  uint32_t v = (_member<<4) | buffer;
  return v << (8*_group);
}

int Pds::Tag::Key::buffer(uint32_t tag) const
{
  unsigned v = tag>>(16+8*_group);
  if ( _member == ((v>>4)&0xf))
    return v&0xf;
  return -1;
}

