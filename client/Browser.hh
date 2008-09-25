#ifndef PDS_BROWSER
#define PDS_BROWSER

#include "InXtcIterator.hh"

namespace Pds {

class Datagram;
class InDatagram;

class Browser : public InXtcIterator
  {
  public:
    Browser(const Datagram&,
	    InDatagramIterator*,
	    int depth,
	    int&);
    Browser(const InXtc& xtc, 
	    InDatagramIterator*,
	    int depth,
	    int&);
    int process(const InXtc&,
		InDatagramIterator*);
  private:
    int _dumpBinaryPayload(const InXtc& inXtc,
			   InDatagramIterator*);
  private:
    int _header;
    int _depth;
  };
}
#endif
