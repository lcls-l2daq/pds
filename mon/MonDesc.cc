#include <string.h>

#include "pds/mon/MonDesc.hh"

using namespace Pds;

MonDesc::MonDesc(const char* name) :
  _id(-1),
  _nentries(0)
{
  strncpy(_name, name, NameSize);
  _name[NameSize-1] = 0;
}

MonDesc::MonDesc(const MonDesc& desc) :
  _id(desc._id),
  _nentries(0)
{
  strncpy(_name, desc._name, NameSize);
}

MonDesc::~MonDesc() {}

const char* MonDesc::name() const {return _name;}
int short MonDesc::id() const {return _id;}
unsigned short MonDesc::nentries() const {return _nentries;}

void MonDesc::added  () {_nentries++;}
void MonDesc::removed() {_nentries--;}
void MonDesc::reset  () {_nentries=0;}
void MonDesc::id(int short i) {_id=i;}
