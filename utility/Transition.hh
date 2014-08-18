#ifndef PDSTRANSITION_HH
#define PDSTRANSITION_HH

#include "pds/collection/Message.hh"
#include "pds/collection/Node.hh"
#include "pds/service/Pool.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/TransitionId.hh"
#include "pdsdata/xtc/Env.hh"

namespace Pds {

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
               uint64_t    bld_mask=0,
	       uint64_t    bld_mask_mon=0,
               unsigned    options=0);
    Allocation(const char* partition,
               const char* dbpath,
	       const char* l3path,
               unsigned    partitionid,
               uint64_t    bld_mask=0,
	       uint64_t    bld_mask_mon=0,
               unsigned    options=0);

    bool add   (const Node& node);
    bool remove(const ProcInfo&);

    unsigned    nnodes() const;
    unsigned    nnodes(Level::Type) const;
    const Node* node(unsigned n) const;
    Node*       node(unsigned n);
    Node*       node(const ProcInfo&);
    const char* partition() const;
    const char* dbpath() const;
    const char* l3path() const;
    unsigned    partitionid() const;
    uint64_t    bld_mask() const;
    uint64_t    bld_mask_mon() const;
    unsigned    options() const;
    unsigned    size() const;
  private:
    static const unsigned MaxNodes=128;
    static const unsigned MaxPName=16;
    static const unsigned MaxName=64;
    static const unsigned MaxDbPath=64;
    char     _partition[MaxPName];
    char     _l3path   [MaxName];
    char     _dbpath   [MaxDbPath];
    uint32_t _partitionid;
    uint32_t _bld_mask[2];
    uint32_t _bld_mask_mon[2];
    uint32_t _nnodes;
    uint32_t _options;
    Node     _nodes[MaxNodes];  // transmit is trunctated at _nnodes of these
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

  class RunInfo : public Transition {
  public:
    RunInfo(unsigned run, unsigned experiment);
    RunInfo(unsigned run, unsigned experiment, char *expname);
    unsigned run();
    unsigned experiment();
    char *expname();
  private:
    static const unsigned MaxExpName=16;
    uint32_t _run;
    uint32_t _experiment;
    char     _expname  [MaxExpName];
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
