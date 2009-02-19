#include <string.h>

#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"

static const unsigned short Step = 16;

using namespace Pds;

MonGroup::MonGroup(const char* name) :
  _desc(name),
  _maxentries(Step),
  _entries(new MonEntry*[_maxentries])
{}

MonGroup::~MonGroup()
{
  unsigned short nentries = _desc.nentries();
  for (unsigned short e=0; e<nentries; e++) {
    delete _entries[e];
  }
  delete [] _entries;
}

void MonGroup::add(MonEntry* entry)
{
  unsigned short used = _desc.nentries();
  if (used == _maxentries) adjust();
  _entries[used] = entry;
  _desc.added();
  entry->desc().id(used);
  entry->desc().group(_desc.id());
}

const MonDesc& MonGroup::desc() const {return _desc;}
MonDesc& MonGroup::desc() {return _desc;}

MonEntry* MonGroup::entry(unsigned short e)
{
  if (e < _desc.nentries())
    return _entries[e];
  else
    return 0;
}

const MonEntry* MonGroup::entry(unsigned short e) const 
{
  if (e < _desc.nentries())
    return _entries[e];
  else
    return 0;
}

const MonEntry* MonGroup::entry(const char* name) const
{
  unsigned short nentries = _desc.nentries();
  for (unsigned short e=0; e<nentries; e++) {
    if (strcmp(_entries[e]->desc().name(), name) == 0)
      return _entries[e];
  }
  return 0;
}

MonEntry* MonGroup::entry(const char* name)
{
  unsigned short nentries = _desc.nentries();
  for (unsigned short e=0; e<nentries; e++) {
    if (strcmp(_entries[e]->desc().name(), name) == 0)
      return _entries[e];
  }
  return 0;
}

unsigned short MonGroup::nentries() const {return _desc.nentries();}

void MonGroup::adjust()
{
  unsigned short maxentries = _maxentries + Step;
  MonEntry** entries = new MonEntry*[maxentries];
  memcpy(entries, _entries, _desc.nentries()*sizeof(MonEntry*));
  delete [] _entries;
  _entries = entries;
  _maxentries = maxentries;
}
