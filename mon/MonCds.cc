#include <string.h>

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/service/Semaphore.hh"

static const unsigned short Step = 16;

using namespace Pds;

//MonCds::MonCds(MonPort::Type type) :
//  _desc(MonPort::name(type)),
MonCds::MonCds(const char* name) :
  _desc(name),
  _maxgroups(Step),
  _groups(new MonGroup*[_maxgroups]),
  _payload_sem(Semaphore::FULL)
{
  //  _desc.id(type);
  _desc.id(0);
}

MonCds::~MonCds()
{
  reset();
}

void MonCds::add(MonGroup* group)
{
  unsigned short used = _desc.nentries();
  if (used == _maxgroups) adjust();
  _groups[used] = group;
  _desc.added();
  group->desc().id(used);
}

//MonPort::Type MonCds::type() const {return MonPort::Type(_desc.id());}

const MonDesc& MonCds::desc() const {return _desc;}

unsigned short MonCds::ngroups() const {return _desc.nentries();}

unsigned short MonCds::totalentries() const 
{
  unsigned short nentries=0;
  for (unsigned short g=0; g<ngroups(); g++) {
    nentries += _groups[g]->desc().nentries();
  }
  return nentries;
}

unsigned short MonCds::totaldescs() const 
{
  unsigned short ndescs=0;
  for (unsigned short g=0; g<ngroups(); g++) {
    ndescs += _groups[g]->desc().nentries();
    ndescs++;
  }
  ndescs++;
  return ndescs;
}

MonGroup* MonCds::group(unsigned short g) const 
{
  if (g < _desc.nentries())
    return _groups[g];
  else
    return 0;
}

MonGroup* MonCds::group(const char* name)
{
  for (unsigned short g=0; g<ngroups(); g++) {
    if (strcmp(_groups[g]->desc().name(), name) == 0)
      return _groups[g];
  }
  return 0;
}

const MonEntry* MonCds::entry(int signature) const 
{
  unsigned short g = signature >> 16;
  unsigned short e = signature & 0xffff;
  if (g < _desc.nentries())
    return _groups[g]->entry(e);
  else
    return 0;
}

MonEntry* MonCds::entry(int signature)
{
  unsigned short g = signature >> 16;
  unsigned short e = signature & 0xffff;
  if (g < _desc.nentries())
    return _groups[g]->entry(e);
  else
    return 0;
}

void MonCds::reset()
{
  if (_groups) {
    unsigned short ngroups = _desc.nentries();
    _desc.reset();
    for (unsigned short g=0; g<ngroups; g++) {
      delete _groups[g];
    }
    delete [] _groups;
    _maxgroups = 0;
    _groups = 0;
  }
}

void MonCds::adjust()
{
  unsigned short maxgroups = _maxgroups + Step;
  MonGroup** groups = new MonGroup*[maxgroups];
  memcpy(groups, _groups, _desc.nentries()*sizeof(MonGroup*));
  delete [] _groups;
  _groups = groups;
  _maxgroups = maxgroups;
}


#include <stdio.h>
#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescEntry.hh"

void MonCds::showentries() const
{
  printf("Serving %d entries:\n", totalentries());
  for (unsigned g=0; g<ngroups(); g++) {
    const MonGroup* gr = group(g);
    printf("%s group\n", gr->desc().name());
    for (unsigned e=0; e<gr->nentries(); e++) {
      const MonEntry* en = gr->entry(e);
      printf("  [%2d] %s\n", e+1, en->desc().name());
    }
  }
}

unsigned MonCds::description(iovec* iov) const
{
  unsigned element=0;
  iov[element].iov_base = (void*)&desc();
  iov[element].iov_len = sizeof(MonDesc);
  element++;
  for (unsigned short g=0; g<ngroups(); g++) {
    MonGroup& gr = *_groups[g];
    iov[element].iov_base = (void*)&gr.desc();
    iov[element].iov_len = sizeof(MonDesc);
    element++;
    for (unsigned short e=0; e<gr.nentries(); e++) {
      const MonEntry* entry = gr.entry(e); 
      iov[element].iov_base = (void*)&entry->desc();
      iov[element].iov_len = entry->desc().size();
      element++;
    }
  }
  return element;
}

Semaphore& MonCds::payload_sem() const
{
  return _payload_sem;
}
