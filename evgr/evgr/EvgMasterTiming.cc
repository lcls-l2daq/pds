#include "EvgMasterTiming.hh"

using namespace Pds;

EvgMasterTiming::EvgMasterTiming(bool     internal,
                                 unsigned fiducials_per_main,
                                 double   main_frequency,
                                 unsigned fiducials_per_beamcode) :
  _internal    (internal),
  _seq_per_main(fiducials_per_main),
  _clks_per_seq(unsigned(119.e6/main_frequency/double(fiducials_per_main))),
  _fid_per_beam(fiducials_per_beamcode)
{
}
