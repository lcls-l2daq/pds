#ifndef Pds_MonMESSAGE_HH
#define Pds_MonMESSAGE_HH

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

    Type type() const;
    void type(Type t);
    unsigned payload() const;
    void payload(const iovec* iov, unsigned iovcnt);
    void payload(unsigned size);

  private:
    Type _type;
    unsigned _payload;
  };
};

#endif
