#include "pds/config/EvrIOFactory.hh"
#include "pds/config/AliasFactory.hh"
#include "pds/config/EvrConfigType.hh"

#include <string>

using namespace Pds;
using Pds::EvrData::OutputMapV2;
using std::vector;
using std::string;

EvrIOFactory::EvrIOFactory() {}

EvrIOFactory::~EvrIOFactory() {}

void EvrIOFactory::insert(unsigned mod_id, unsigned chan, const DetInfo& info)
{
  OutputMapV2 output(OutputMapV2::Pulse,-1,
		     OutputMapV2::UnivIO,chan,mod_id);
  
  MapType::iterator it = _map.find(output.value());
  if (it == _map.end())
    (_map[output.value()] = vector<DetInfo>()).push_back(info);
  else
    it->second.push_back(info);
}

EvrIOConfigType* EvrIOFactory::config(const AliasFactory& aliases)
{
  vector<EvrIOChannelType> ch;
  for(MapType::iterator it=_map.begin(); it!=_map.end(); it++) {
    string name;
    unsigned ninfo = it->second.size();
    for(unsigned i=0; i<ninfo; i++) {
      const char* p=aliases.lookup(it->second[i]);
      if (!p) p=DetInfo::name(it->second[i]);
      if (name.size()+strlen(p)+1<EvrIOChannelType::NameLength) {
	if (name.size()) name += string(",");
	name += string(p);
      }
    }
    name.resize      (EvrIOChannelType::NameLength);
    vector<DetInfo> infos = it->second;
    infos.resize(EvrIOChannelType::MaxInfos  ,infos[0]);
    ch.push_back(EvrIOChannelType(reinterpret_cast<const OutputMapV2&>(it->first),
				  name.c_str(),
				  ninfo,
				  infos.data()));
  }

  EvrIOConfigType t(ch.size());
  return new (new char[t._sizeof()]) EvrIOConfigType(ch.size(),ch.data());
}
