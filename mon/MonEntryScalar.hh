#ifndef Pds_MonENTRYScalar_HH
#define Pds_MonENTRYScalar_HH

#include "pds/mon/MonEntry.hh"
#include "pds/mon/MonDescScalar.hh"

#include <vector>

namespace Pds {

  class MonEntryScalar : public MonEntry {
  public:
    MonEntryScalar(const MonDescScalar& desc);

    virtual ~MonEntryScalar();

    void    params(const MonDescScalar&);

    double  value() const;
    double  value(unsigned element) const;
    std::vector<double> values() const;

    std::vector<double> since(const MonEntryScalar&) const;

    void   addvalue(double);
    void   addvalue(double,unsigned);
    void   setvalue(double);
    void   setvalue(double,unsigned);
    void   setvalues(const double*);
    void   setto(const MonEntryScalar&);

    // Implements MonEntry
    virtual const MonDescScalar& desc() const;
    virtual MonDescScalar& desc();

  private:
    MonDescScalar _desc;

  private:
    double* _y;
  };

  inline double MonEntryScalar::value() const { return *_y; }

  inline double MonEntryScalar::value(unsigned i) const { return _y[i]; }

  inline void   MonEntryScalar::addvalue(double v) { _y[0]+=v; }

  inline void   MonEntryScalar::addvalue(double v,unsigned i) { _y[i]+=v; }

  inline void   MonEntryScalar::setvalue(double v) { _y[0]=v; }

  inline void   MonEntryScalar::setvalue(double v,unsigned i) { _y[i]=v; }
};

#endif
