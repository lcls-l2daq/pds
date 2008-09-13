#ifndef PDS_EBSERVER
#define PDS_EBSERVER

#include "pds/service/Server.hh"

#include "pds/xtc/xtc.hh"
#include "EbKey.hh"

namespace Pds {

class Ins;

class EbServer : public Server
  {
  public:
    EbServer(const Src& client, unsigned id, int fd);
   ~EbServer();
  private:
    //  Implements unused parts of Server
    char* payload();
    int   commit(char* datagram, char* payload, int size, const Ins&);
    int   handleError(int value);
  public:
    int  drops() const;
    int  fixup();
    void keepAlive();
    const Src& client() const;
  public:
    enum {KeepAlive = 3};
  public:
    //  Eb interface
    virtual void        dump    (int detail)   const = 0;
    virtual unsigned    keyTypes()             const = 0;
    virtual const char* key     (EbKey::Type)  const = 0;
    virtual bool        matches (const EbKey&) const = 0;
    virtual bool        isValued()             const = 0;
    //  EbSegment interface
    virtual const InXtc&   xtc   () const = 0;
    virtual bool           more  () const = 0;
    virtual unsigned       length() const = 0;
    virtual unsigned       offset() const = 0;
  private:
    int _keepAlive;       // # of contineous drops
    int _drop;            // # of of contributions dropped
    Src _client;          // Identity of client being served
  };
}

/*
** ++
**
**    Each server represents its client. If a client does not, for a
**    a particular event contribute its fraction of data to the event,
**    this function is called with the event who has not seen a
**    contriubtion from our client. The function increments its counter of
**    dropped contribtions calls the event's fixup function, passing
**    to it both our client's ID and a tagged container used to "dummy up"
**    the client's contribution. The function decrements and returns its
**    keep-alive signal. Therefore, if the function returns a zero (0)
**    value, the client being serviced by this class can be considered
**    "dead", conversly, a non-zero value indicates that the client's
**    fate is still in some doubt.
**
** --
*/

inline int Pds::EbServer::fixup()
  {
   _drop++;
  return _keepAlive--;
  }

/*
** ++
**
**   Returns the number of times the client proxied by this server
**   has not seen an expected contribution.
**
** --
*/

inline int Pds::EbServer::drops() const
  {
  return _drop;
  }

/*
** ++
**
**    Get the identity of the client representing this server...
**
** --
*/

inline const Pds::Src& Pds::EbServer::client() const
  {
  return _client;
  }

/*
** ++
**
**    Each server has a value which is decremented by it's "fixup" function
**    and to its initial value by this function. In this fashion it behaves
**    as a "keep-alive" value, used by its manager ("Eb") to determine
**    whether the client being served by this class is "alive".
**
** --
*/

inline void Pds::EbServer::keepAlive()
  {
  _keepAlive = KeepAlive;
  }

#endif
