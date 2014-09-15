#ifndef Pds_GenericPgp_PeriodMonitor_hh
#define Pds_GenericPgp_PeriodMonitor_hh

#include <time.h>

namespace Pds {
  namespace GenericPgp {
    class PeriodMonitor {
    public:
      PeriodMonitor(unsigned);
      ~PeriodMonitor();
    public:
      void clear();
      void start();
      void event(unsigned);
      void print() const;
    private:
      unsigned                       _size;
      unsigned*                      _histo;
      timespec                       _thisTime;
      timespec                       _lastTime;
      bool                           _first;
      unsigned                       _timeSinceLastException;
      unsigned                       _fetchesSinceLastException;
    };
  };
};

#endif
