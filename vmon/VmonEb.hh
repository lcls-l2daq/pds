#ifndef Pds_VmonEb_hh
#define Pds_VmonEb_hh

namespace Pds {

  class ClockTime;
  class Src;
  class MonServerManager;
  class MonEntryTH1F;

  class VmonEb {
  public:
    VmonEb(const Src&,
	   unsigned nservers,
	   unsigned maxdepth,
	   unsigned maxtime,
	   unsigned maxsize);
    ~VmonEb();
  public:
    bool time_fetch() const { return _fetch_time!=0; }
  public:
    void fixup     (int server);
    void depth     (unsigned events);
    void fetch_time(unsigned ticks);
    void damage_count(unsigned dmg);
    void post_time (unsigned ticks);
    void post_size (unsigned bytes);
    void update    (const ClockTime&);
  private:
    MonEntryTH1F*     _fixup;
    MonEntryTH1F*     _depth;
    MonEntryTH1F*     _fetch_time;
    MonEntryTH1F*     _damage_count;
    MonEntryTH1F*     _post_time;
    MonEntryTH1F*     _post_time_log;
    MonEntryTH1F*     _post_size;
    unsigned          _tshift;
    unsigned          _sshift;
  };

};

#endif
