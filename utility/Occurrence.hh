#ifndef PDS_OCCURRENCE_HH
#define PDS_OCCURRENCE_HH

#include "pds/collection/Message.hh"
#include "pds/utility/OccurrenceId.hh"
#include "pdsdata/xtc/Sequence.hh"

namespace Pds {
  class Pool;
  class Occurrence : public Message
  {
  public:
    Occurrence(OccurrenceId::Value,
	       unsigned size = sizeof(Occurrence));
    Occurrence(const Occurrence&);

    OccurrenceId::Value id      ()   const;

    void* operator new(size_t size, Pool* pool);
    void  operator delete(void* buffer);
  private:
    OccurrenceId::Value _id;
  };

  class DataFileOpened : public Occurrence
  {
  public:
    DataFileOpened(unsigned _expt,
		   unsigned _run,
		   unsigned _stream,
		   unsigned _chunk,
		   char* _host,
		   char* _path);
  public:
    enum { MaxHostLength=40 };
    enum { MaxPathLength=200 };
    unsigned expt;
    unsigned run;
    unsigned stream;
    unsigned chunk;
    char host[MaxHostLength];
    char path[MaxPathLength];
  };

  class DataFileError : public Occurrence
  {
  public:
    DataFileError(unsigned _expt,
		   unsigned _run,
		   unsigned _stream,
		   unsigned _chunk ) :
      Occurrence(OccurrenceId::DataFileError,
		 sizeof(DataFileError)),
      expt  (_expt  ),
      run   (_run   ),
      stream(_stream),
      chunk (_chunk )
    {}
  public:
    unsigned expt;
    unsigned run;
    unsigned stream;
    unsigned chunk;
  };

  class UserMessage : public Occurrence
  {
  public:
    enum { MaxMsgLength=8000 };
    UserMessage();
    UserMessage(const char* msg);
  public:
    const char* msg() const { return _msg; }
    int         remaining() const;
    void append(const char*);
  private:
    char _msg[MaxMsgLength];
  };

  class EvrCommand : public Occurrence
  {
  public:
    EvrCommand(const Sequence& _seq, unsigned char _code) : 
      Occurrence(OccurrenceId::EvrCommand,sizeof(EvrCommand)), 
      seq (_seq ),
      code(_code) 
    {}
  public:
    Sequence seq;
    unsigned code;
  };
}

#endif
