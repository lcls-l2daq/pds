#ifndef Pds_MonSTATS2D_HH
#define Pds_MonSTATS2D_HH

namespace Pds {

  class MonStats2D {
  public:
    MonStats2D();
    ~MonStats2D();

    void stats(unsigned nbinsx, unsigned nbinsy, 
	       float xlo, float xup, 
	       float ylo, float yup, 
	       const float* w);

    double meanx() const;
    double rmsx() const;
    double meany() const;
    double rmsy() const;

    double sum() const;
    double sumx() const;
    double sumy() const;
    double sumx2() const;
    double sumy2() const;
    double sumxy() const;
     
  private:
    double _sumw; 
    double _sumw2; 
    double _sumwx; 
    double _sumwx2;
    double _sumwy;
    double _sumwy2;
    double _sumwxy;
  };
};

#endif
