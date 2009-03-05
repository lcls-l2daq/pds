#ifndef Pds_CollectionObserver_hh
#define Pds_CollectionObserver_hh

#include "pds/collection/CollectionManager.hh"

namespace Pds {

  class CollectionObserver : public CollectionManager {
  public:
    CollectionObserver(unsigned char platform) :
      CollectionManager(Level::Observer, platform, MaxPayload, ConnectTimeOut, NULL) {}
    ~CollectionObserver() {}
  public:
    virtual void post(const Transition&) = 0;
  private:
    void message(const Node& hdr, const Message& msg)
    {
      if (hdr.level() == Level::Control && msg.type()==Message::Transition) {
        const Transition& tr = reinterpret_cast<const Transition&>(msg);
	if (tr.phase() == Transition::Execute)
	  post(tr);
      }
    }
  };

};

#endif
