#include "pds/vmon/VmonEb.hh"

#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonDescScalar.hh"
#include "pds/utility/EbServer.hh"
#include "pds/collection/Node.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/BldInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include <time.h>
#include <math.h>

using namespace Pds;

static const int tbins_pwr_max = 6; // maximum of 64 time bins
static const int fetch_shift = 8;

static int time_scale(unsigned  maxtime,
		      unsigned& maxtime_q)
{
  int tshift = 0;
  maxtime_q  = 1;
  while(maxtime_q < maxtime) {
    maxtime_q <<= 1;
    tshift++;
  }
  return (tshift > tbins_pwr_max) ? tshift-tbins_pwr_max : 0;
}


VmonEb::VmonEb(const Src& src,
	       unsigned nservers,
	       unsigned maxdepth,
	       unsigned maxtime,
	       unsigned maxsize,
	       const char* group_name)
{
  MonGroup* group = new MonGroup(group_name);
  VmonServerManager::instance()->cds().add(group);

  char tmp[32];
  std::vector<std::string> srv_names(nservers);
  for(unsigned i=0; i<nservers; i++) {
    sprintf(tmp,"srv%02d",i);
    srv_names[i] = std::string(tmp);
  }

  MonDescScalar fixup("Fixups",srv_names);
  _fixup = new MonEntryScalar(fixup);
  group->add(_fixup);

  MonDescTH1F depth("Depth", "events", "",
		    maxdepth+1, -0.5, float(maxdepth)+0.5);
  _depth = new MonEntryTH1F(depth);
  group->add(_depth);

  unsigned maxt;
  _tshift = time_scale(maxtime, maxt);

  float t0 = -0.5*1.e-3;
  float t1 = (float(maxt)-0.5)*1.e-3;

  MonDescTH1F post_time("Post Time", "[us]", "",
			maxt>>_tshift, t0, t1);
  _post_time = new MonEntryTH1F(post_time);
  group->add(_post_time);

  //
  //  Add new log(t) histogram
  //
  const int logt_bins = 32;
  const float lt0 = 6.; // 1ms
  const float lt1 = 9; //  1s
  MonDescTH1F post_time_log("Log Post Time", "log10 [ns]", "",
			    logt_bins, lt0, lt1);
  _post_time_log = new MonEntryTH1F(post_time_log);
  group->add(_post_time_log);

  { unsigned maxf;
    _fshift = time_scale(maxtime>>8, maxf);
    
    float ft0 = -0.5*1.e-3;
    float ft1 = (float(maxf)-0.5)*1.e-3;
    MonDescTH1F fetch_time("Fetch Time", "[us]", "",
                           maxf>>_fshift,ft0,ft1);
    _fetch_time = new MonEntryTH1F(fetch_time);
    //    group->add(_fetch_time);
  }

  { unsigned maxf;
    _lshift = time_scale(maxtime>>2, maxf);
    
    float ft0 = -0.5*1.e-3;
    float ft1 = (float(maxf)-0.5)*1.e-3;
    MonDescTH1F fetch_time("Fetch Time Long", "[us]", "",
                           maxf>>_lshift,ft0,ft1);
    _fetch_time_long = new MonEntryTH1F(fetch_time);
    //    group->add(_fetch_time_long);
  }

  std::vector<std::string> bit_names(32);
  for(unsigned i=0; i<32; i++) {
    sprintf(tmp,"b%02d",i);
    bit_names[i] = std::string(tmp);
  }

  MonDescScalar damage_count("Damage", bit_names);
  _damage_count = new MonEntryScalar(damage_count);
  group->add(_damage_count);

  unsigned maxs;
  _sshift = time_scale(maxsize,maxs);
  float s0 = -0.5;
  float s1 = float(maxs)-0.5;

  MonDescTH1F post_size("Post Size","[bytes]", "",
			maxs>>_sshift, s0, s1);
  _post_size = new MonEntryTH1F(post_size);
  group->add(_post_size);
}

VmonEb::~VmonEb()
{
}

void VmonEb::fixup(int server)
{
  if (server<0) return;
  unsigned bin = server;
  if (bin < _fixup->desc().elements())
    _fixup->addvalue(1, bin);
}

void VmonEb::depth(unsigned events)
{
  if (events < _depth->desc().nbins())
    _depth->addcontent(1, events);
  else
    _depth->addinfo(1, MonEntryTH1F::Overflow);
}

void VmonEb::post_time(unsigned t)
{
  unsigned bin = t>>_tshift;
  if (bin < _post_time->desc().nbins())
    _post_time->addcontent(1, bin);
  else
    _post_time->addinfo(1, MonEntryTH1F::Overflow);

  _post_time_log->addcontent(1., log10f(double(t)));
}

void VmonEb::fetch_time(unsigned t)
{
  unsigned bin = t>>_fshift;
  if (bin < _fetch_time->desc().nbins())
    _fetch_time->addcontent(1, bin);
  else
    _fetch_time->addinfo(1, MonEntryTH1F::Overflow);

  bin = t>>_lshift;
  if (bin < _fetch_time_long->desc().nbins())
    _fetch_time_long->addcontent(1, bin);
  else
    _fetch_time_long->addinfo(1, MonEntryTH1F::Overflow);
}

// damage histogram
void VmonEb::damage_count(unsigned dmg)
{
  unsigned bin;

  // increment a bin for each 1 bit in 'dmg'
  for (bin = 0; dmg; bin ++, dmg >>= 1) {
    if (dmg & 1) {
      if (bin < _damage_count->desc().elements())
        _damage_count->addvalue(1, bin);
    }
  }
}

void VmonEb::post_size(unsigned s)
{
  unsigned bin = s>>_sshift;
  if (bin < _post_size->desc().nbins())
    _post_size->addcontent(1, bin);
  else
    _post_size->addinfo(1, MonEntryTH1F::Overflow);
}

void VmonEb::update(const ClockTime& now)
{
  _fixup     ->time(now);
  _depth     ->time(now);
  _post_time ->time(now);
  _post_time_log ->time(now);
  _fetch_time->time(now);
  _fetch_time_long->time(now);
  _damage_count->time(now);
  _post_size ->time(now);
}

void VmonEb::server(const Server& srv)
{
  const EbServer& e = static_cast<const EbServer&>(srv);
  std::vector<std::string> names=_fixup->desc().get_names();

  switch(e.client().level()) {
  case Level::Source:
    names[e.id()]=std::string(DetInfo::name(static_cast<const DetInfo&>(e.client()))); 
    break;
  case Level::Reporter:
    names[e.id()]=std::string(BldInfo::name(static_cast<const BldInfo&>(e.client()))); 
  default:
    { char* buff = new char[256];
      Node::ip_name(static_cast<const ProcInfo&>(e.client()).ipAddr(),buff,256);
      names[e.id()]=std::string(buff);
      delete[] buff;
    } break;
  }

  _fixup->desc().set_names(names);
}
