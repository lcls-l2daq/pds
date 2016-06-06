#ifndef PDS_BROWSER
#define PDS_BROWSER

#include "pdsdata/xtc/XtcIterator.hh"

namespace Pds {

class Datagram;
class InDatagram;

  class Browser : public XtcIterator
  {
  public:
    Browser(const Datagram&,
	    int depth);
    Browser(const Xtc& xtc, 
	    int depth);
    int process(Xtc*);

    static void setDumpLength(unsigned);
  private:
    int _dumpBinaryPayload(const Xtc& xtc);
  private:
    int _header;
    int _depth;
  };
}
#endif
