#ifndef Pds_VmonRecord_hh
#define Pds_VmonRecord_hh

#include "pdsdata/xtc/ClockTime.hh"

#include <vector>
using std::vector;

namespace Pds {

  class MonCds;
  class MonClient;
  class Src;

  class VmonRecord {
  public:
    enum Type { Description, Payload };
    VmonRecord() {}
    VmonRecord(Type             type,
	       const ClockTime& time,
	       unsigned         len=sizeof(VmonRecord));
    ~VmonRecord();
  public:
    int len() const;
    const ClockTime& time() const;
  public:
    int  append(const MonClient& client);
    void append(const MonClient& client,int);
    void time  (const ClockTime&);
  public:
    int extract(vector<Src    >& src_vector, 
		vector<MonCds*>& cds_vector, 
		vector<int*   >& offset_vector) ;
  private:
    Type      _type;
    ClockTime _time;
  protected:
    unsigned  _len;
  };
};

#endif
