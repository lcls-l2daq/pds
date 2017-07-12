#ifndef MonTag_Client_hh
#define MonTag_Client_hh

namespace Pds {
  class Ins;
  namespace MonTag {
    class Client {
    public:
      Client(const Ins&  svc,
             const char* group);
    public:
      void     request      (unsigned buffer);
    private:
      int      _socket;
    };
  };
};

#endif
