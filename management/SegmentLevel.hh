#ifndef Pds_SegmentLevel_hh
#define Pds_SegmentLevel_hh

#include "PartitionMember.hh"

namespace Pds {

  class SegWireSettings;
  class SegStreams;
  class Arp;
  class EventCallback;
  class EbIStream;
  class EvrServer;

  class SegmentLevel: public PartitionMember {
  public:
    SegmentLevel(unsigned platform,
		 SegWireSettings& settings,
		 EventCallback& callback,
		 Arp* arp);

    virtual ~SegmentLevel();

    bool attach();
    void detach();
  private:
    // Implements PartitionMember
    Message& reply     (Message::Type);
    void     allocated (const Allocation&, unsigned);
    void     dissolved ();
    void     post      (const Transition&);
    void     post      (const InDatagram&);

  private:
    SegWireSettings& _settings;
    EventCallback& _callback;
    SegStreams*    _streams;
    EbIStream*     _inlet;
    EvrServer*     _evr;
    Message        _reply;
  };

}
#endif
