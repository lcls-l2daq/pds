#ifndef Pds_VmonEb_hh
#define Pds_VmonEb_hh

namespace Pds {

  class Src;
  class MonServerManager;
  class MonEntryTH1F;

  class VmonEb {
  public:
    VmonEb(const Src&,
	   unsigned nservers,
	   unsigned maxdepth,
	   unsigned maxtime);
    ~VmonEb();
  public:
    bool time_fetch() const { return _fetch_time!=0; }
  public:
    void fixup     (int server);
    void depth     (unsigned events);
    void post_time (unsigned ticks);
    void fetch_time(unsigned ticks);
    void update    ();
  private:
    MonEntryTH1F*     _fixup;
    MonEntryTH1F*     _depth;
    MonEntryTH1F*     _post_time;
    MonEntryTH1F*     _fetch_time;
    unsigned          _tshift;
  };

};

#endif
