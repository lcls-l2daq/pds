#ifndef PDS_OCCURRENCE_HH
#define PDS_OCCURRENCE_HH

#include "pds/collection/Message.hh"
#include "pds/utility/OccurrenceId.hh"

namespace Pds {
  class Pool;
  class Occurrence : public Message
  {
  public:
    Occurrence(OccurrenceId::Value,
	       unsigned size = sizeof(Occurrence));
    OccurrenceId::Value id      ()   const;

    void* operator new(size_t size, Pool* pool);
    void  operator delete(void* buffer);
  private:
    OccurrenceId::Value _id;
  };
}

#endif
