#include "pds/vmon/VmonEb.hh"

#include "pds/vmon/VmonServerManager.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonDescTH1F.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"

#include <time.h>

using namespace Pds;

static const int tbins_pwr_max = 6; // maximum of 64 time bins

VmonEb::VmonEb(const Src& src,
	       unsigned nservers,
	       unsigned maxdepth,
	       unsigned maxtime)
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

  int tshift = 0;
  unsigned maxt   = 1;
  while(maxt < maxtime) {
    maxt <<= 1;
    tshift++;
  }
  _tshift = (tshift > tbins_pwr_max) ? tshift-tbins_pwr_max : 0;

  MonDescTH1F post_time("Post Time", "ticks", "",
			maxt>>_tshift, -0.5, float(maxt)-0.5);
  _post_time = new MonEntryTH1F(post_time);
  group->add(_post_time);

  if (!maxtime) {
    _fetch_time=0;
    return;
  }

  MonDescTH1F fetch_time("Fetch Time", "ticks", "",
			 maxt>>_tshift, -0.5, float(maxt)-0.5);
  _fetch_time = new MonEntryTH1F(fetch_time);
  group->add(_fetch_time);
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
}

void VmonEb::fetch_time(unsigned t)
{
  unsigned bin = t>>_tshift;
  if (bin < _fetch_time->desc().nbins())
    _fetch_time->addcontent(1, bin);
  else
    _fetch_time->addinfo(1, MonEntryTH1F::Overflow);
}

void VmonEb::update()
{
  struct timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);
  ClockTime now(tv.tv_sec,tv.tv_nsec);
  _fixup->time(now);
  _depth->time(now);
  _post_time->time(now);
  if (_fetch_time) _fetch_time->time(now);
}
