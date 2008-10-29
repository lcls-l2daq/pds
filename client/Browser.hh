#ifndef PDS_BROWSER
#define PDS_BROWSER

#include "XtcIterator.hh"

namespace Pds {

class Datagram;
class InDatagram;

class Browser : public XtcIterator
  {
  public:
    Browser(const Datagram&,
	    InDatagramIterator*,
	    int depth,
	    int&);
    Browser(const Xtc& xtc, 
	    InDatagramIterator*,
	    int depth,
	    int&);
    int process(const Xtc&,
		InDatagramIterator*);
  private:
    int _dumpBinaryPayload(const Xtc& xtc,
			   InDatagramIterator*);
  private:
    int _header;
    int _depth;
  };
}
#endif
