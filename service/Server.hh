#ifndef PDS_SERVER
#define PDS_SERVER

#include "LinkedList.hh"

namespace Pds {

class ZcpDatagram;

class Server : public LinkedList<Server>
  {
  public:
    virtual ~Server();
  public:
    Server(unsigned id,
	   int      fd);
  public:
    unsigned id() const;
    int      fd() const;

    void id(unsigned);
    void fd(int);

    virtual int      pend        (int flag = 0)             = 0;

    virtual int      fetch       (char* payload, int flags) = 0;
    //    virtual int      fetchHeader (int flags)                = 0;
    //    virtual int      fetchPayload(char* payload, int flags) = 0;

    //    int      fetch(ZcpDatagram& , int flags);

    //    virtual const char* datagram() const = 0;
  private:
    unsigned      _id;             // Policy "neutral" identification.
    int           _fd;
  };
}


inline Pds::Server::Server(unsigned id,
			   int      fd) :
  _id(id),
  _fd(fd)
{
}

inline Pds::Server::~Server()
{
}

inline unsigned Pds::Server::id() const
{
  return _id;
}

inline int      Pds::Server::fd() const
{
  return _fd;
}

inline void Pds::Server::id(unsigned v)
{
  _id = v;
}

inline void Pds::Server::fd(int v)
{
  _fd = v;
}

#endif
