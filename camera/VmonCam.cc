#include "pds/camera/VmonCam.hh"

#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/vmon/VmonServerManager.hh"
#include "pdsdata/xtc/ClockTime.hh"

using namespace Pds;

VmonCam::VmonCam(unsigned nbuffers)
{
  Pds::MonGroup* group = new Pds::MonGroup("Cam");
  Pds::VmonServerManager::instance()->cds().add(group);

  Pds::MonDescTH1F depth("Depth", "buffers", "", nbuffers+1, -0.5, double(nbuffers)+0.5);
  _depth = new Pds::MonEntryTH1F(depth);
  group->add(_depth);

  float t_ms = float(1<<20)*1.e-6;
  Pds::MonDescTH1F interval("Interval", "[ms]", "", 64, -0.25*t_ms, 31.75*t_ms);
  _interval = new Pds::MonEntryTH1F(interval);
  group->add(_interval);

  Pds::MonDescTH1F interval_w("Interval wide", "[ms]", "", 64, -2.*t_ms, 254.*t_ms);
  _interval_w = new Pds::MonEntryTH1F(interval_w);
  group->add(_interval_w);

  _sample_sec  = 0;
  _sample_nsec = 0;
}

VmonCam::~VmonCam()
{
  delete _depth;
  delete _interval;
  delete _interval_w;
}

void VmonCam::event(unsigned depth)
{
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  unsigned dsec(tp.tv_sec - _sample_sec);
  if (dsec<2) {
    unsigned nsec(tp.tv_nsec);
    if (dsec==1) nsec += 1000000000;
    unsigned b = (nsec-_sample_nsec)>>19;
    if (b < _interval->desc().nbins())
      _interval->addcontent(1.,b);
    else
      _interval->addinfo(1.,Pds::MonEntryTH1F::Overflow);
    b >>= 3;
    if (b < _interval_w->desc().nbins())
      _interval_w->addcontent(1.,b);
    else
      _interval_w->addinfo(1.,Pds::MonEntryTH1F::Overflow);
  }
  _sample_sec  = tp.tv_sec;
  _sample_nsec = tp.tv_nsec;

  _depth->addcontent(1.,depth);

  Pds::ClockTime clock(tp.tv_sec,tp.tv_nsec);
  _interval_w->time(clock);
  _interval  ->time(clock);
  _depth     ->time(clock);
}
