#ifndef Pds_VmonReader_hh
#define Pds_VmonReader_hh

#include "pds/mon/MonCds.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/Src.hh"

#include <vector>
using std::vector;

namespace Pds {

  class MonStats1D;
  class MonStats2D;
  class MonUsage;
  class MonCds;

  class VmonReaderCallback {
  public:
    virtual ~VmonReaderCallback() {}
  public:
    virtual void process(const ClockTime&, 
			 const Src&, 
			 int signature, 
			 const MonStats1D&) = 0;
    virtual void process(const ClockTime&, 
			 const Src&, 
			 int signature, 
			 const MonStats2D&) = 0;
  };

  class VmonReader {
  public:
    VmonReader(const char* name);
    ~VmonReader();
  public:
    const vector<Src>& sources() const;
    const MonCds* cds(const Src&) const;
    const ClockTime& begin() const;
    const ClockTime& end  () const;
  public:    
    void reset();
    void use  (const Src&, const MonUsage&);
  public:
    void process(VmonReaderCallback&);
    void process(VmonReaderCallback&,
		 const ClockTime& begin,
		 const ClockTime& end);
  private:
    char*                _buff;
    FILE*                _file;
    //  contents
    std::vector<Src>       _src;
    std::vector<MonCds*>   _cds;
    std::vector<int*>      _offsets;
    unsigned             _seek_pos; // start of payload
    unsigned             _len;      // length of each payload record

    ClockTime            _begin;
    ClockTime            _end;
    //  requests
    std::vector<Src>             _req_src;
    std::vector<const MonUsage*> _req_use;
    std::vector<int*>            _req_off;
  };

};

#endif
