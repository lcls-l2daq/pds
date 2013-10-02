#ifndef PDS_BROWSER
#define PDS_BROWSER

#include "pds/client/XtcIterator.hh"

namespace Pds {

class Datagram;
class InDatagram;

  class Browser : public PdsClient::XtcIterator
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

    static void setDumpLength(unsigned);
  private:
    int _dumpBinaryPayload(const Xtc& xtc,
			   InDatagramIterator*);
  private:
    int _header;
    int _depth;
  };
}
#endif
