#ifndef Pds_MonMESSAGE_HH
#define Pds_MonMESSAGE_HH

#include "pdsdata/xtc/Src.hh"

class iovec;

namespace Pds {

  class MonMessage {
  public:
    enum Type{NoOp,
	      Reset,
	      Disabled,
	      Enabled,
	      DescriptionReq, 
	      Description, 
	      PayloadReq, 
	      Payload};

    MonMessage(Type type, unsigned payload=0);
    MonMessage(const Src& src, Type type, unsigned payload=0);

    Src  src () const;
    Type type() const;
    void type(Type t);
    unsigned payload() const;
    void payload(const iovec* iov, unsigned iovcnt);
    void payload(unsigned size);

  private:
    Src  _src;
    Type _type;
    unsigned _payload;
  };
};

#endif
