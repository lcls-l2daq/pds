#ifndef PDSTRANSITION_HH
#define PDSTRANSITION_HH

#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"
#include "pds/service/Pool.hh"
#include "pds/service/BldBitMaskSize.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pdsdata/xtc/Env.hh"

#include <list>
#include <vector>

namespace Pds {
  template <unsigned N> class BitMaskArray;
  typedef BitMaskArray<PDS_BLD_MASKSIZE> BldBitMask;

  class Transition : public Message {
  public:
    enum Phase { Execute, Record };
    static const char* name(TransitionId::Value id);

  public:
    Transition(TransitionId::Value id,
               Phase           phase,
               const Sequence& sequence,
               const Env&      env, 
               unsigned        size=sizeof(Transition));

    Transition(TransitionId::Value id,
               const Env&          env, 
               unsigned            size=sizeof(Transition));

    Transition(const Transition&);

    TransitionId::Value id      () const;
    Phase           phase   () const;
    const Sequence& sequence() const;
    const Env&      env     () const;

    void* operator new(size_t size, Pool* pool);
    void* operator new(size_t size);
    void  operator delete(void* buffer);
  private:
    uint32_t     _id;
    Phase        _phase;
    Sequence     _sequence;
    Env          _env;
    uint32_t     _reserved; // maintain 8-byte alignment?
  private:
    void         _stampIt();
    friend class TimeStampApp;
  };

  class Allocation {
  public:
    enum { ShapeTmo       =1,
           ShortDisableTmo=2,
           L3Tag          =4,
           L3Veto         =8 };
    Allocation();
    Allocation(const char* partition,
               const char* dbpath,
               unsigned    partitionid,
               unsigned    masterid=0,
               const BldBitMask* bld_mask=0,
               const BldBitMask* bld_mask_mon=0,
               unsigned    options=0);
    Allocation(const char* partition,
               const char* dbpath,
               const char* l3path,
               unsigned    partitionid,
               unsigned    masterid=0,
               const BldBitMask* bld_mask=0,
               const BldBitMask* bld_mask_mon=0,
               unsigned    options=0,
               float       unbiased_fraction=0.);

    bool add   (const Node& node);
    bool remove(const ProcInfo&);

    void set_l3t  (const char* path, bool veto=false, float unbiased_f=0);
    void clear_l3t();

    unsigned    nnodes() const;
    unsigned    nnodes(Level::Type) const;
    const Node* node(unsigned n) const;
    Node*       node(unsigned n);
    Node*       node(const ProcInfo&);
    const Node* master() const;
    const char* partition() const;
    const char* dbpath() const;
    const char* l3path() const;
    unsigned    partitionid() const;
    unsigned    masterid() const;
    BldBitMask  bld_mask() const;
    BldBitMask  bld_mask_mon() const;
    unsigned    options() const;
    unsigned    size() const;
    bool        l3tag () const;
    bool        l3veto() const;
    float       unbiased_fraction() const;
    float       traffic_interval() const;  // seconds
    void        dump() const;
  public:
    static void set_traffic_interval(float);
  private:
    static const unsigned MaxNodes=128;
    static const unsigned MaxPName=16;
    static const unsigned MaxName=128;
    static const unsigned MaxDbPath=64;
    char     _partition[MaxPName];
    char     _l3path   [MaxName];
    char     _dbpath   [MaxDbPath];
    uint32_t _partitionid;
    uint32_t _masterid;
    uint32_t _bld_mask[PDS_BLD_MASKSIZE];
    uint32_t _bld_mask_mon[PDS_BLD_MASKSIZE];
    uint32_t _nnodes;
    uint32_t _options;
    float    _unbiased_f;
    float    _traffic_interval;
    Node     _nodes[MaxNodes];  // transmit is truncated at _nnodes of these
  };

  class Allocate : public Transition {
  public:
    Allocate(const Allocation&);
    Allocate(const Allocation&,
             const Sequence&);
  public:
    const Allocation& allocation() const;
  private:
    Allocation _allocation;
  };

  class SegPayload {
  public:
    SegPayload() : info(Level::Segment, 0, 0), offset(0) {}
    ProcInfo info;
    unsigned offset;
  };

  class RunInfo : public Transition {
  public:
    RunInfo();
    RunInfo(const std::list<SegPayload>&);
    RunInfo(unsigned run, unsigned experiment);
    RunInfo(unsigned run, unsigned experiment, char *expname);
    RunInfo(const std::list<SegPayload>&, unsigned run, unsigned experiment);
    RunInfo(const std::list<SegPayload>&, unsigned run, unsigned experiment, char *expname);
  public:
    bool     recording () const;
    unsigned run       () const;
    unsigned experiment() const;
    const char* expname() const;
    int      offset    (const ProcInfo&) const;
    std::vector<SegPayload> payloads() const;
  private:
    static const unsigned MaxExpName=16;
    static const unsigned MaxSegs=64;
    uint32_t   _run;
    uint32_t   _experiment;
    char       _expname  [MaxExpName];
    unsigned   _nsegs;
    SegPayload _payload[MaxSegs];
  };

  class Kill : public Transition {
  public:
    Kill(const Node& allocator);

    const Node& allocator() const;
  
  private:
    Node _allocator;
  };

}

#endif
