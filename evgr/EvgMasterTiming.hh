#ifndef Pds_EvgMasterTiming_hh
#define Pds_EvgMasterTiming_hh

namespace Pds {

  class EvgMasterTiming {
  public:
    EvgMasterTiming(bool     internal, 
                    unsigned fiducials_per_main, 
                    double   main_frequency);
    
    bool     internal_main      () const { return _internal; }
    unsigned sequences_per_main () const { return _seq_per_main; }
    unsigned clocks_per_sequence() const { return _clks_per_seq; }
  private:
    bool     _internal;
    unsigned _seq_per_main;
    unsigned _clks_per_seq;
  };
}

#endif /* PDSEVGRPULSEPARAMS_HH_ */
