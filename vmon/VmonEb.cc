#include "pds/vmon/VmonEb.hh"

#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"

#include <time.h>
#include <math.h>

using namespace Pds;

static const int tbins_pwr_max = 6; // maximum of 64 time bins

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
	       unsigned maxsize)
{
  MonGroup* group = new MonGroup("Eb");
  VmonServerManager::instance()->cds().add(group);

  MonDescTH1F fixup("Fixups", "server", "", 
		    nservers+1, -1.5, float(nservers)-0.5);
  _fixup = new MonEntryTH1F(fixup);
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

  MonDescTH1F fetch_time("Fetch Time", "[us]", "",
			 maxt>>_tshift, t0, t1);
  _fetch_time = new MonEntryTH1F(fetch_time);
  group->add(_fetch_time);

  MonDescTH1F damage_count("Damage", "bit #", "",
			 32, -0.5, 31.5);
  _damage_count = new MonEntryTH1F(damage_count);
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
  unsigned bin = server+1;
  if (bin < _fixup->desc().nbins())
    _fixup->addcontent(1, unsigned(server+1));
  else
    _fixup->addinfo(1, MonEntryTH1F::Overflow);
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
  unsigned bin = t>>_tshift;
  if (bin < _fetch_time->desc().nbins())
    _fetch_time->addcontent(1, bin);
  else
    _fetch_time->addinfo(1, MonEntryTH1F::Overflow);
}

// damage histogram
void VmonEb::damage_count(unsigned dmg)
{
  unsigned bin;

  // increment a bin for each 1 bit in 'dmg'
  for (bin = 0; dmg; bin ++, dmg >>= 1) {
    if (dmg & 1) {
      if (bin < _damage_count->desc().nbins())
        _damage_count->addcontent(1, bin);
      else
        _damage_count->addinfo(1, MonEntryTH1F::Overflow);
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
  _damage_count->time(now);
  _post_size ->time(now);
}
