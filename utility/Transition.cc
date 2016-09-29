#include "Transition.hh"

#include "pds/service/BldBitMask.hh"

#include <string.h>
#include <time.h>
#include <stdio.h>

static float _default_traffic_interval = 0.008;

using namespace Pds;

static Sequence now(TransitionId::Value id)
{
  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  uint64_t pulseId = -1UL;
  ClockTime clocktime(tp.tv_sec, tp.tv_nsec);
  TimeStamp timestamp(pulseId, 0);
  return Sequence(Sequence::Event, id, clocktime, timestamp);
}

static Sequence none(TransitionId::Value id)
{
  return Sequence(Sequence::Event, id, ClockTime(-1, -1), TimeStamp(-1UL,0) );
}

Transition::Transition(TransitionId::Value id,
                       Phase           phase,
                       const Sequence& sequence,
                       const Env&      env, 
                       unsigned        size) :
  Message(Message::Transition, size),
  _id      (id),
  _phase   (phase),
  _sequence(sequence),
  _env     (env)
{}

Transition::Transition(TransitionId::Value id,
                       const Env&          env, 
                       unsigned            size) :
  Message  (Message::Transition, size),
  _id      (id),
  _phase   (Execute),
  _sequence(none(id)),
  _env     (env)
{}

Transition::Transition(const Transition& c) :
  Message(Message::Transition, c.reply_to(), c.size()),
  _id      (c._id),
  _phase   (c._phase),
  _sequence(c._sequence),
  _env     (c._env)
{
  memcpy(this+1,&c+1,c.size()-sizeof(Transition));
}

TransitionId::Value Transition::id() const {return TransitionId::Value(_id);}

Transition::Phase Transition::phase() const {return _phase;}

const Sequence& Transition::sequence() const { return _sequence; }

const Env& Transition::env() const {return _env;}

void* Transition::operator new(size_t size, Pool* pool)
{
  return pool->alloc(size);
}

void* Transition::operator new(size_t size)
{
  PoolEntry* entry = 
    reinterpret_cast<PoolEntry*>(::new char[size+sizeof(PoolEntry)]);
  entry->_pool = 0;
  return entry+1;
}

void Transition::operator delete(void* buffer)
{
  PoolEntry* entry = PoolEntry::entry(buffer);
  if (entry->_pool) {
    Pool::free(buffer);
  } else {
    delete [] reinterpret_cast<char*>(entry);
  }
}

void Transition::_stampIt()
{
  _sequence = now(id());
}

Allocation::Allocation() :
  _partitionid(0),
  _nnodes     (0),
  _options    (0),
  _unbiased_f (0)
{
  _bld_mask[0] = _bld_mask[1] = 0;
  _bld_mask_mon[0] = _bld_mask_mon[1] = 0;
}

Allocation::Allocation(const char*       partition,
                       const char*       dbpath,
                       unsigned          partitionid,
                       unsigned          masterid,
                       const BldBitMask* bld_mask,
                       const BldBitMask* bld_mask_mon,
                       unsigned          options) :
  _partitionid(partitionid),
  _masterid   (masterid),
  _nnodes     (0),
  _options    (options),
  _unbiased_f (0),
  _traffic_interval(_default_traffic_interval)
{
  strncpy(_partition, partition, MaxPName-1);
  strncpy(_dbpath   , dbpath   , MaxDbPath-1);
  _l3path[0] = 0;
  for (unsigned i=0; i<PDS_BLD_MASKSIZE; i++) {
    if (bld_mask) {
      _bld_mask[i] = bld_mask->value(i);
      _bld_mask_mon[i] = bld_mask_mon->value(i);
    } else {
      _bld_mask[i] = 0;
      _bld_mask_mon[i] = 0;
    }
  }
}

Allocation::Allocation(const char*       partition,
                       const char*       dbpath,
                       const char*       l3path,
                       unsigned          partitionid,
                       unsigned          masterid,
                       const BldBitMask* bld_mask,
                       const BldBitMask* bld_mask_mon,
                       unsigned          options,
		       float unbiased_fraction) : 
  _partitionid(partitionid),
  _masterid   (masterid),
  _nnodes     (0),
  _options    (options),
  _unbiased_f (unbiased_fraction),
  _traffic_interval(_default_traffic_interval)
{
  strncpy(_partition, partition, MaxPName-1);
  strncpy(_l3path   , l3path   , MaxName-MaxPName-1);
  strncpy(_dbpath   , dbpath   , MaxDbPath-1);
  for (unsigned i=0; i<PDS_BLD_MASKSIZE; i++) {
    if (bld_mask) {
      _bld_mask[i] = bld_mask->value(i);
      _bld_mask_mon[i] = bld_mask_mon->value(i);
    } else {
      _bld_mask[i] = 0;
      _bld_mask_mon[i] = 0;
    }
  }
}

bool Allocation::add   (const Node& node)
{
  if (_nnodes < MaxNodes) {
    _nodes[_nnodes++] = node;
    return true;
  } else {
    return false;
  }
}

bool Allocation::remove(const ProcInfo& info)
{
  for(unsigned j=0; j<_nnodes; j++)
    if (_nodes[j].procInfo() == info) {
      _nodes[j] = _nodes[--_nnodes];
      return true;
    }
  return false;
}

void Allocation::set_l3t(const char* path, bool veto, float unbiased_f)
{
  strncpy(_l3path, path, MaxName);
  
  _options |= L3Tag;
  if (veto) {
    _options   |= L3Veto;
    _unbiased_f = unbiased_f;
  }
  else
    _options  &= ~L3Veto;
}

void Allocation::clear_l3t()
{
  _options &= ~(L3Tag|L3Veto);
}

void Allocation::set_traffic_interval(float v)
{ _default_traffic_interval=v; }

unsigned Allocation::partitionid() const { return _partitionid; }

unsigned Allocation::masterid() const { return _masterid; }

unsigned Allocation::nnodes() const {return _nnodes;}

const Node* Allocation::node(unsigned n) const 
{
  if (n < _nnodes) {
    return _nodes+n;
  } else {
    return 0;
  }
}

Node* Allocation::node(unsigned n)
{
  if (n < _nnodes) {
    return _nodes+n;
  } else {
    return 0;
  }
}

Node* Allocation::node(const ProcInfo& info)
{
  for(unsigned j=0; j<_nnodes; j++)
    if (_nodes[j].procInfo() == info) {
      return &_nodes[j];
    }
  return 0;
}

const Node* Allocation::master() const
{
  for(unsigned j=0; j<_nnodes; j++)
    if (_nodes[j].level() == Level::Segment) {
      return &_nodes[j];
    }
  return 0;
}

unsigned Allocation::nnodes(Level::Type level) const
{
  unsigned n=0;
  for(unsigned i=0; i<_nnodes; i++)
    if (_nodes[i].level()==level)
      n++;
  return n;
}

BldBitMask Allocation::bld_mask() const 
{
  BldBitMask mask = BldBitMask();
  for (unsigned i=0; i<PDS_BLD_MASKSIZE; i++) {
    mask.setValue(i, _bld_mask[i]);
  }
  return mask;
}

BldBitMask Allocation::bld_mask_mon() const 
{
  BldBitMask mask = BldBitMask();
  for (unsigned i=0; i<PDS_BLD_MASKSIZE; i++) {
    mask.setValue(i, _bld_mask_mon[i]);
  }
  return mask;
}

unsigned Allocation::options() const
{
  return _options;
}

const char* Allocation::partition() const {return _partition;}

const char* Allocation::dbpath() const {return _dbpath;}

const char* Allocation::l3path() const {return _l3path;}

float       Allocation::unbiased_fraction() const { return _unbiased_f; }

float       Allocation::traffic_interval() const { return _traffic_interval; }

unsigned    Allocation::size() const { return sizeof(*this)+(_nnodes-MaxNodes)*sizeof(Node); }

bool        Allocation::l3tag() const { return _options&L3Tag; }

bool        Allocation::l3veto() const { return _options&L3Veto; }

void        Allocation::dump() const
{
  printf("Partition: %s\n", _partition);
  printf("L3Path   : %s\n", _l3path);
  printf("DbPath   : %s\n", _dbpath);
  printf("Part Id  : %u\n", _partitionid);
  printf("Master Id: %u\n", _masterid);
  printf("Bld Mask : 0x%08x%08x\n", _bld_mask[1], _bld_mask[0]);
  printf("Bld Mask Mon : 0x%08x%08x\n", _bld_mask_mon[1], _bld_mask_mon[0]);
  printf("NNodes   : %u\n", _nnodes);
  printf("Options  : 0x%x\n", _options);
  printf("UnbiasedFrac: %g\n", _unbiased_f);
  printf("Traffic Interval: %g\n", _traffic_interval);
  for(unsigned i=0; i<_nnodes; i++) {
    const Node& no = _nodes[i];
    printf("%s %08x.%08x %08x\n", 
           Level::name(no.level()),
           no.procInfo().log(),
           no.procInfo().phy(),
           no.paddr());
  }
}

Allocate::Allocate(const Allocation& allocation) :
  Transition(TransitionId::Map, Transition::Execute, none(TransitionId::Map), 0,
             sizeof(Allocate)+allocation.size()-sizeof(Allocation)),
  _allocation(allocation)
{
}

Allocate::Allocate(const Allocation& allocation,
		   const Sequence& seq) :
  Transition(TransitionId::Map, Transition::Execute, seq, 0,
             sizeof(Allocate)+allocation.size()-sizeof(Allocation)),
  _allocation(allocation)
{
}

const Allocation& Allocate::allocation() const
{ return _allocation; }

static const unsigned PayloadAlignment = 256;

RunInfo::RunInfo() :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             time(0),
             sizeof(RunInfo)),
  _run(0),
  _experiment(0),
  _nsegs(0)
{
  _expname[0] = '\0';
}

RunInfo::RunInfo(const std::list<SegPayload>& payload) :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             time(0),
             sizeof(RunInfo)),
  _run(0),
  _experiment(0),
  _nsegs(0)
{
  _expname[0] = '\0';
  std::list<SegPayload>::const_iterator it=payload.begin(); 
  while(it!=payload.end() && _nsegs<MaxSegs)
    _payload[_nsegs++] = *it++;
  if (it!=payload.end()) {}
}

RunInfo::RunInfo(unsigned                     run, 
                 unsigned                     experiment) :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             run,
             sizeof(RunInfo)),
  _run(run),
  _experiment(experiment), 
  _nsegs(0)
{
  _expname[0] = '\0';
}

RunInfo::RunInfo(unsigned                     run, 
                 unsigned                     experiment, 
                 char*                        expname) :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             run,
             sizeof(RunInfo)),
  _run(run),
  _experiment(experiment), 
  _nsegs(0)
{
  strncpy(_expname, expname, MaxExpName-1);
}

RunInfo::RunInfo(const std::list<SegPayload>& payload, 
                 unsigned                     run, 
                 unsigned                     experiment) :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             run,
             sizeof(RunInfo)),
  _run(run),
  _experiment(experiment), 
  _nsegs(0)
{
  _expname[0] = '\0';
  std::list<SegPayload>::const_iterator it=payload.begin(); 
  while(it!=payload.end() && _nsegs<MaxSegs)
    _payload[_nsegs++] = *it++;
  if (it!=payload.end()) {}
}

RunInfo::RunInfo(const std::list<SegPayload>& payload, 
                 unsigned                     run, 
                 unsigned                     experiment, 
                 char*                        expname) :
  Transition(TransitionId::BeginRun, 
             Transition::Execute,
             none(TransitionId::BeginRun), 
             run,
             sizeof(RunInfo)),
  _run(run),
  _experiment(experiment), 
  _nsegs(0)
{
  strncpy(_expname, expname, MaxExpName-1);
  std::list<SegPayload>::const_iterator it=payload.begin(); 
  while(it!=payload.end() && _nsegs<MaxSegs)
    _payload[_nsegs++] = *it++;
  if (it!=payload.end()) {}
}

bool     RunInfo::recording () const {return _run;}

unsigned RunInfo::run       () const {return _run;}

unsigned RunInfo::experiment() const {return _experiment;}

const char* RunInfo::expname() const {return _expname;}

int      RunInfo::offset    (const ProcInfo& info) const 
{
  unsigned p=0;
  for(unsigned i=0; i<_nsegs; i++) {
    if (info == _payload[i].info)
      return p;
    p += (_payload[i].offset+PayloadAlignment-1)&~(PayloadAlignment-1);
  }
  return -1;
}

std::vector<SegPayload> RunInfo::payloads() const
{
  std::vector<SegPayload> v(_nsegs);
  for(unsigned i=0; i<_nsegs; i++)
    v[i] = _payload[i];
  return v;
}

Kill::Kill(const Node& allocator) : 
  Transition(TransitionId::Unmap, Transition::Execute, none(TransitionId::Unmap), 0, 
             sizeof(Kill)),
  _allocator(allocator)
{}

const Node& Kill::allocator() const 
{
  return _allocator;
}
