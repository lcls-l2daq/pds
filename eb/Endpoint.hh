#ifndef Pds_Fabrics_Endpoint_hh
#define Pds_Fabrics_Endpoint_hh

#include <rdma/fabric.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_rma.h>

#include <vector>
#include <poll.h>
#include <sys/uio.h>

struct fi_msg_rma;                      // RiC hack

namespace Pds {
  namespace Fabrics {
    enum State { EP_CLOSED, EP_INIT, EP_UP, EP_LISTEN, EP_CONNECTED };

    class RemoteAddress {
    public:
      RemoteAddress();
      RemoteAddress(uint64_t rkey, uint64_t addr, size_t extent);
      const struct fi_rma_iov* rma_iov() const;
    public:
      uint64_t addr;
      size_t extent;
      uint64_t rkey;
    };

    class MemoryRegion {
    public:
      MemoryRegion(struct fid_mr* mr, void* start, size_t len);
      ~MemoryRegion();
      uint64_t rkey() const;
      uint64_t addr() const;
      void* desc() const;
      void* start() const;
      size_t length() const;
      struct fid_mr* fid() const;
      bool contains(void* start, size_t len) const;
    private:
      struct fid_mr* _mr;
      void*          _start;
      size_t         _len;
    };

    class LocalAddress {
    public:
      LocalAddress();
      LocalAddress(void* buf, size_t len, MemoryRegion* mr=NULL);
      void* buf() const;
      size_t len() const;
      MemoryRegion* mr() const;
      const struct iovec* iovec() const;
    public:
      void*         _buf;
      size_t        _len;
      MemoryRegion* _mr;
    };

    class LocalIOVec {
    public:
      LocalIOVec(size_t count, LocalAddress** local_addrs=NULL);
      LocalIOVec(const std::vector<LocalAddress*>& local_addrs);
      ~LocalIOVec();
      bool check_mr();
      size_t count() const;
      const struct iovec* iovecs() const;
      void** desc() const;
      bool set_iovec(unsigned index, LocalAddress* local_addr);
      bool set_iovec(unsigned index, void* buf, size_t len, MemoryRegion* mr=NULL);
      bool set_iovec_mr(unsigned index, MemoryRegion* mr);
    private:
      void verify();
    private:
      bool          _mr_set;
      size_t        _count;
      struct iovec* _iovs;
      void**        _mr_desc;
    };

    class RemoteIOVec {
    public:
      RemoteIOVec(size_t count, RemoteAddress** remote_addrs=NULL);
      RemoteIOVec(const std::vector<RemoteAddress*>& remote_addrs);
      ~RemoteIOVec();
      size_t count() const;
      const struct fi_rma_iov* iovecs() const;
      bool set_iovec(unsigned index, RemoteAddress* remote_addr);
      bool set_iovec(unsigned index, uint64_t rkey, uint64_t addr, size_t extent);
    private:
      size_t              _count;
      struct fi_rma_iov*  _rma_iovs;
    };

    class RmaMessage {
    public:
      RmaMessage();
      RmaMessage(LocalIOVec* loc_iov, RemoteIOVec* rem_iov, void* context, uint64_t data=0);
      ~RmaMessage();
      const struct fi_msg_rma* msg() const;
    private:
      struct fi_msg_rma* _msg;
    };

    class ErrorHandler {
    public:
      ErrorHandler();
      virtual ~ErrorHandler();
      int error_num() const;
      const char* error() const;
      void clear_error();
    protected:
      void set_custom_error(const char* fmt, ...);
      void set_error(const char* error_desc);
    protected:
      int   _errno;
      char* _error;
    };

    class Fabric : public ErrorHandler {
    public:
      Fabric(const char* node, const char* service, int flags=0);
      ~Fabric();
      MemoryRegion* register_memory(void* start, size_t len);
      MemoryRegion* lookup_memory(void* start, size_t len) const;
      MemoryRegion* lookup_memory(LocalAddress* laddr) const;
      bool lookup_memory_iovec(LocalIOVec* iov) const;
      bool up() const;
      struct fi_info* info() const;
      struct fid_fabric* fabric() const;
      struct fid_domain* domain() const;
    private:
      bool initialize(const char* node, const char* service, int flags);
      void shutdown();
    private:
      bool                        _up;
      struct fi_info*             _hints;
      struct fi_info*             _info;
      struct fid_fabric*          _fabric;
      struct fid_domain*          _domain;
      std::vector<MemoryRegion*>  _mem_regions;
    };

    class EndpointBase : public ErrorHandler {
    protected:
      EndpointBase(const char* addr, const char* port, int flags=0);
      EndpointBase(Fabric* fabric);
      virtual ~EndpointBase();
    public:
      State state() const;
      Fabric* fabric() const;
      struct fid_eq* eq() const;
      struct fid_cq* cq() const;
      virtual void shutdown();
      bool event(uint32_t* event, void* entry, bool* cm_entry);
      bool event_wait(uint32_t* event, void* entry, bool* cm_entry, int timeout=-1);
      bool event_error(struct fi_eq_err_entry *entry);
    protected:
      bool handle_event(ssize_t event_ret, bool* cm_entry, const char* cmd);
      bool initialize();
    protected:
      State                       _state;
      const bool                  _fab_owner;
      Fabric*                     _fabric;
      struct fid_eq*              _eq;
      struct fid_cq*              _cq;
    };

    class Endpoint : public EndpointBase {
    public:
      Endpoint(const char* addr, const char* port, int flags=0);
      Endpoint(Fabric* fabric);
      ~Endpoint();
    public:
      void shutdown();
      bool connect();
      bool accept(struct fi_info* remote_info);
      bool comp(struct fi_cq_data_entry* comp, int* comp_num, ssize_t max_count);
      bool comp_wait(struct fi_cq_data_entry* comp, int* comp_num, ssize_t max_count, int timeout=-1);
      bool comp_error(struct fi_cq_err_entry* comp_err);
      /* Asynchronous calls (raw buffer) */
      bool recv_comp_data(void* context);
      bool send(void* buf, size_t len, void* context, const MemoryRegion* mr=NULL);
      bool recv(void* buf, size_t len, void* context, const MemoryRegion* mr=NULL);
      bool read(void* buf, size_t len, const RemoteAddress* raddr, void* context, const MemoryRegion* mr=NULL);
      bool write(void* buf, size_t len, const RemoteAddress* raddr, void* context, const MemoryRegion* mr=NULL);
      bool write_data(void* buf, size_t len, const RemoteAddress* raddr, void* context, uint64_t data, const MemoryRegion* mr=NULL);
      /* Asynchronous calls (LocalAddress wrapper) */
      bool send(LocalAddress* laddr, void* context);
      bool recv(LocalAddress* laddr, void* context);
      bool read(LocalAddress* laddr, const RemoteAddress* raddr, void* context);
      bool write(LocalAddress* laddr, const RemoteAddress* raddr, void* context);
      bool write_data(LocalAddress* laddr, const RemoteAddress* raddr, void* context, uint64_t data);
      /* Vectored Asynchronous calls */
      bool sendv(LocalIOVec* iov, void* context);
      bool recvv(LocalIOVec* iov, void* context);
      bool readv(LocalIOVec* iov, const RemoteAddress* raddr, void* context);
      bool writev(LocalIOVec* iov, const RemoteAddress* raddr, void* context);
      /* Synchronous calls (raw buffer) */
      bool recv_comp_data_sync(uint64_t* data);
      bool send_sync(void* buf, size_t len, const MemoryRegion* mr=NULL);
      bool recv_sync(void* buf, size_t len, const MemoryRegion* mr=NULL);
      bool read_sync(void* buf, size_t len, const RemoteAddress* raddr, const MemoryRegion* mr=NULL);
      bool write_sync(void* buf, size_t len, const RemoteAddress* raddr, const MemoryRegion* mr=NULL);
      bool write_data_sync(void* buf, size_t len, const RemoteAddress* raddr, uint64_t data, const MemoryRegion* mr=NULL);
      /* Synchronous calls (LocalAddress wrapper) */
      bool send_sync(LocalAddress* laddr);
      bool recv_sync(LocalAddress* laddr);
      bool read_sync(LocalAddress* laddr, const RemoteAddress* raddr);
      bool write_sync(LocalAddress* laddr, const RemoteAddress* raddr);
      bool write_data_sync(LocalAddress* laddr, const RemoteAddress* raddr, uint64_t data);
      /* Vectored Synchronous calls */
      bool sendv_sync(LocalIOVec* iov);
      bool recvv_sync(LocalIOVec* iov);
      bool readv_sync(LocalIOVec* iov, const RemoteAddress* raddr);
      bool writev_sync(LocalIOVec* iov, const RemoteAddress* raddr);
      bool write_msg(const fi_msg_rma*, unsigned flags); // RiC hack
    private:
      bool handle_comp(ssize_t comp_ret, struct fi_cq_data_entry* comp, int* comp_num, const char* cmd);
      bool check_completion(int context, unsigned flags);
      bool check_connection_state();
    private:
      uint64_t        _counter;
      struct fid_ep*  _ep;
    };

    class PassiveEndpoint : public EndpointBase {
    public:
      PassiveEndpoint(const char* addr, const char* port, int flags=0);
      ~PassiveEndpoint();
    public:
      void shutdown();
      bool listen();
      Endpoint* accept();
      bool reject();
      bool close(Endpoint* endpoint);
    private:
      int                     _flags;
      struct fid_pep*         _pep;
      std::vector<Endpoint*>  _endpoints;
    };

    class CompletionPoller : public ErrorHandler {
    public:
      CompletionPoller(Fabric* fabric, nfds_t size_hint=1);
      ~CompletionPoller();
      bool up() const;
      bool add(Endpoint* endp);
      bool del(Endpoint* endp);
      bool poll(int timeout=-1);
      void shutdown();
    private:
      bool initialize();
      void check_size();
    private:
      bool          _up;
      Fabric*       _fabric;
      nfds_t        _nfd;
      nfds_t        _nfd_max;
      pollfd*       _pfd;
      struct fid**  _pfid;
      Endpoint**    _endps;
    };
  }
}

#endif
