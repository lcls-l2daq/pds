#ifndef PDS_ASSIGNMENT_HH
#define PDS_ASSIGNMENT_HH

#include "pds/collection/Message.hh"

namespace Pds {

  //
  //  Collection message for communicating stream port assigments
  //
  class StreamPortAssignment : public Message {
  public:
    StreamPortAssignment(Message::Type, const Ins&);

    void* operator new(unsigned,void* p) { return p; }
    void  operator delete(void*) {}

    const Ins& client() const;

    Ins& client();
    void add_server(const Ins&);
  private:
    Ins _client;
  };

  //
  //  Iterator for extracting the server port assignments
  //
  class StreamPortAssignmentIter {
  public:
    StreamPortAssignmentIter(const StreamPortAssignment&);
    bool end() const;
    const Ins& operator()() const;
    StreamPortAssignmentIter& operator++();
  private:
    const Ins* _next;
    const Ins* _end;
  };

}

inline Pds::StreamPortAssignment::StreamPortAssignment(Pds::Message::Type type,
						       const Pds::Ins& s) : 
  Pds::Message(type, sizeof(*this)), 
  _client(s) 
{
}

inline void Pds::StreamPortAssignment::add_server(const Pds::Ins& c)
{
  Pds::Ins* s = reinterpret_cast<Pds::Ins*>(reinterpret_cast<char*>(this)+size());
  *s = c;
  _size += sizeof(Ins);
}

inline const Pds::Ins& Pds::StreamPortAssignment::client() const 
{
  return _client; 
}


inline Pds::Ins& Pds::StreamPortAssignment::client() 
{
  return _client; 
}


inline Pds::StreamPortAssignmentIter::StreamPortAssignmentIter(const Pds::StreamPortAssignment& a) :
  _next(reinterpret_cast<const Pds::Ins*>(&a+1)),
  _end (reinterpret_cast<const Pds::Ins*>(reinterpret_cast<const char*>(&a)+a.size()))
{
}

inline bool Pds::StreamPortAssignmentIter::end() const 
{ return _next==_end; }

inline const Pds::Ins& Pds::StreamPortAssignmentIter::operator()() const 
{ return *_next; }

inline Pds::StreamPortAssignmentIter& Pds::StreamPortAssignmentIter::operator++() 
{ _next++; return *this; }

#endif
