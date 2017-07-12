#ifndef PdsTag_Client_hh
#define PdsTag_Client_hh

#include "pds/tag/Key.hh"

namespace Pds {
  class Ins;
  namespace Tag {
    class Client {
    public:
      Client(const Ins& server,
             unsigned   platform,
             unsigned   group);
      ~Client();
    public:
      const Key& key() const;
    public:
      void request(unsigned buffer);
    private:
      int     _fd;
      Key     _key;
    };
  };
};

#endif
