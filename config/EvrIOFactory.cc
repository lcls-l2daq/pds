#include "pds/config/EvrIOFactory.hh"
#include "pds/config/AliasFactory.hh"
#include "pds/config/EvrConfigType.hh"

#include <vector>
#include <string>

using namespace Pds;
using Pds::EvrData::OutputMapV2;
using std::vector;
using std::string;
using std::list;

EvrIOFactory::EvrIOFactory() {}

EvrIOFactory::~EvrIOFactory() {}

void EvrIOFactory::insert(unsigned mod_id, unsigned chan, const DetInfo& info)
{
  OutputMapV2 output(OutputMapV2::Pulse,-1,
		     OutputMapV2::UnivIO,chan,mod_id);
  
  MapType::iterator it = _map.find(output.value());
  if (it == _map.end())
    (_map[output.value()] = list<DetInfo>()).push_back(info);
  else {
    it->second.push_back(info);
    it->second.sort();
    it->second.unique();
  }
}

EvrIOConfigType* EvrIOFactory::config(const AliasFactory& aliases,
				      const PartitionConfigType* partn) const
{
  vector<EvrIOChannelType> ch;
  for(MapType::const_iterator it=_map.begin(); it!=_map.end(); it++) {
    string name;
    vector<DetInfo> infos;
    for(list<DetInfo>::const_iterator dit=it->second.begin();
	dit!=it->second.end(); dit++) {
      bool lskip=(partn!=0);
      if (partn) {
	ndarray<const Partition::Source,1> s(partn->sources());
	for(unsigned i=0; i<s.size(); i++)
	  if (*dit==static_cast<const DetInfo&>(s[i].src())) {
	    lskip=false; 
	    break;
	  }
      }
      if (!lskip) {
	infos.push_back(*dit);
	const char* p=aliases.lookup(*dit);
	if (!p) p=DetInfo::name(*dit);
	if (name.size()+strlen(p)+1<EvrIOChannelType::NameLength) {
	  if (name.size()) name += string(",");
	  name += string(p);
	}
      }
    }
    unsigned ninfo = infos.size();
    if (ninfo) {
      name.resize      (EvrIOChannelType::NameLength);
      infos.resize(EvrIOChannelType::MaxInfos  ,*infos.begin());
      ch.push_back(EvrIOChannelType(reinterpret_cast<const OutputMapV2&>(it->first),
				    name.c_str(),
				    ninfo,
				    infos.data()));
    }
  }

  EvrIOConfigType t(ch.size());
  return new (new char[t._sizeof()]) EvrIOConfigType(ch.size(),ch.data());
}
