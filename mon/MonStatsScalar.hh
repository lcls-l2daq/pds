#ifndef Pds_MonSTATSScalar_HH
#define Pds_MonSTATSScalar_HH

#include <vector>

namespace Pds {
  class MonDesc;

  class MonStatsScalar {
  public:
    //    MonStatsScalar();
    MonStatsScalar(unsigned, const double*);
    MonStatsScalar(const std::vector<double>&);
    MonStatsScalar(const MonStatsScalar&);
    ~MonStatsScalar();

    void reset();
    void setto(const MonStatsScalar&);
    void setto(const MonStatsScalar&, const MonStatsScalar&);

    unsigned      elements() const { return _n; }
    const double* values  () const { return reinterpret_cast<const double*>(this+1); }

    unsigned size() const;

  public:
    static unsigned size(const MonDesc&);
  private:
    unsigned _n;
  };
};

#endif
