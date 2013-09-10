#ifndef Pds_MonSTATS1D_HH
#define Pds_MonSTATS1D_HH

namespace Pds {

  class MonStats1D {
  public:
    MonStats1D();
    ~MonStats1D();

    void stats(unsigned nbins, float xlo, float xup, const double* con);
    void setto(const MonStats1D&);
    void setto(const MonStats1D&, const MonStats1D&);

    double sum() const;
    double mean() const;
    double rms() const;
    double under() const;
    double over() const;
    double sumx() const;
    double sumx2() const;

  private:
    double _sumw; 
    double _sumw2; 
    double _sumwx; 
    double _sumwx2;
    double _under;
    double _over;
  };
};

#endif
