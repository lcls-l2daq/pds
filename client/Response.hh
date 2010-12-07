#ifndef PdsResponse_hh
#define PdsResponse_hh

namespace Pds {
  class Occurrence;

  class Response {
  public:
    virtual ~Response() {}
    virtual Occurrence* fire(Occurrence* tr) { return 0; }
  };
}

#endif
