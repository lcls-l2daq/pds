#ifndef PDS_NETSERVER
#define PDS_NETSERVER

#include "Port.hh"
#include "OobServer.hh"

namespace Pds {

class NetServer : public Port, public OobServer
  {
  public:
    virtual ~NetServer();
  public:
    NetServer(unsigned id,
	      const Ins&,
	      int sizeofDatagram,
	      int maxPayload,
	      int maxDatagrams = 40);
    NetServer(unsigned id,
	      int sizeofDatagram,
	      int maxPayload,
	      int maxDatagrams = 40);
  public:
    int       socket() const;
  public:
    virtual int      pend        (int flag = 0);
    virtual int      fetch       (char* payload, int flags);
    //    virtual int      fetchHeader (int flags);
    //    virtual int      fetchPayload(char* payload, int flags);

    //    int      fetch(ZcpDatagram& , int flags);
  public:
    const char* datagram() const;
  private:
    virtual char*    payload();
    virtual int      commit(char* datagram,
			    char* payload,
			    int payloadSize,
			    const Ins&);
    virtual int      handleError(int value);
  public:
    int      unblock(char* datagram, char* payload=(char*)0, int size=0);
  public:
    int      join(const Ins& group, const Ins& interface);
    int      resign();
  protected:
    NetServer(unsigned id,
              Port::Type,
              const Ins&,
              int sizeofDatagram,
              int maxPayload,
              int maxDatagrams);
  private:
    enum {SendFlags = 0};
  private:
    void _construct(int sizeofDatagram, int maxPayload);
  private:
    char*              _datagram;       // -> buffer for current  datagram
    char**             _payload;        // Pointer to -> buffer current payload
    struct msghdr      _hdr;            // Control structure socket receive 
    struct msghdr      _zhdr;           // Control structure socket receive for zero-copy fetches
    struct iovec       _iov[2];         // Buffer description socket receive 
    struct sockaddr_in _src;            // Socket name source machine
    int                _maxPayload;     // Maximum payload size
    int                _sizeofDatagram; // Size of datagram

    unsigned           _mcastGroup;
    unsigned           _mcastInterface;

#ifdef ODF_LITTLE_ENDIAN
    char* _swap_buffer;
    void _swap(int length);
#endif
  };
}


inline int Pds::NetServer::socket() const
  {
  return (int)_socket;
  }

inline const char* Pds::NetServer::datagram() const
  {
  return _datagram;
  }

#endif
