#ifndef PDS_ASSIGNMENT_HH
#define PDS_ASSIGNMENT_HH

#include "pds/collection/Message.hh"
#include "pds/xtc/xtc.hh"

namespace Pds {

  class StreamPort {
  public:
    //    StreamPort() : ins() {}
    StreamPort(const Ins& _ins, const Src& _src) : ins(_ins), src(_src) {}
  public:
    Ins ins;    // receiving sockaddr
    Src src;
  };
    
  //
  //  Collection message for communicating stream port assigments
  //
  class StreamPortAssignment : public Message {
  public:
    StreamPortAssignment(Message::Type type) :
      Message(type, sizeof(*this)) {}

    void* operator new(unsigned,void* p) { return p; }
    void  operator delete(void*) {}

    void add_server(const StreamPort&);
  };

  //
  //  Iterator for extracting the server port assignments
  //
  class StreamPortAssignmentIter {
  public:
    StreamPortAssignmentIter(const StreamPortAssignment&);
    bool end() const;
    const StreamPort& operator()() const;
    StreamPortAssignmentIter& operator++();
  private:
    const StreamPort* _next;
    const StreamPort* _end;
  };

}


inline void Pds::StreamPortAssignment::add_server(const Pds::StreamPort& c)
{
  Pds::StreamPort* s = reinterpret_cast<Pds::StreamPort*>(reinterpret_cast<char*>(this)+size());
  *s = c;
  _size += sizeof(StreamPort);
}

inline Pds::StreamPortAssignmentIter::StreamPortAssignmentIter(const Pds::StreamPortAssignment& a) :
  _next(reinterpret_cast<const Pds::StreamPort*>(&a+1)),
  _end (reinterpret_cast<const Pds::StreamPort*>(reinterpret_cast<const char*>(&a)+a.size()))
{
}

inline bool Pds::StreamPortAssignmentIter::end() const 
{ return _next==_end; }

inline const Pds::StreamPort& Pds::StreamPortAssignmentIter::operator()() const 
{ return *_next; }

inline Pds::StreamPortAssignmentIter& Pds::StreamPortAssignmentIter::operator++() 
{ _next++; return *this; }

#endif
