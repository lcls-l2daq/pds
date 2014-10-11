#include "CfgClientNfs.hh"

#include "pds/config/XtcClient.hh"
#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#define DBUG

using namespace Pds;

#ifdef DBUG
static inline double time_diff(struct timespec& b, 
                               struct timespec& e)
{
  return double(e.tv_sec - b.tv_sec) + 1.e-9*
    (double(e.tv_nsec) - double(b.tv_nsec));
}
#endif

static void clear_map(std::map<unsigned,CfgClientNfs::CacheEntry>& m)
{
  for(std::map<unsigned,CfgClientNfs::CacheEntry>::iterator it=m.begin();
      it!=m.end(); it++)
    if (it->second.size > 0) delete[] it->second.p;
  m.clear();
}

CfgClientNfs::CfgClientNfs( const Src& src ) :
  _src( src ),
  _db ( 0 ),
  _key( 0 )
{
}

CfgClientNfs::~CfgClientNfs()
{
  if (_db) delete _db;
  clear_map(_cache);
}

const Src& CfgClientNfs::src() const
{ return _src; }

void CfgClientNfs::initialize(const Allocation& alloc)
{
  std::string path(alloc.dbpath());
  if (_path != path) {
    clear_map(_cache);
    _path = path;
    _key  = 0;

    //
    //  Maybe opening new connections is slow
    //
    if (_db) delete _db;
    _db = Pds_ConfigDb::XtcClient::open(alloc.dbpath());
  }
}

int CfgClientNfs::fetch(const Transition& tr, 
			const TypeId&     id, 
			void*             dst,
			unsigned          maxSize)
{
  Pds_ConfigDb::XtcClient* db = _db;

  if (!db) {
#ifdef DBUG
    printf("%s line %d: db == %p\n", __FILE__, __LINE__, db);
#endif
    return 0;   // ERROR
  }

  //
  //  New key is not a guarantee that data has changed.
  //  Could consider caching "name" of data returned to
  //  supply back to XtcClient as shortcut.
  //
  if (_key != tr.env().value())
    clear_map(_cache);

  _key = tr.env().value();

  CacheEntry& c = _cache[id.value()];
  if (c.size > 0)
    memcpy(dst,c.p,c.size);
  else {
#ifdef DBUG
    struct timespec tv_b;
    clock_gettime(CLOCK_REALTIME,&tv_b);
#endif

    int result = db->getXTC(tr.env().value(),
			    _src,
			    id,
			    dst,
			    maxSize);
#ifdef DBUG
    struct timespec tv_e;
    clock_gettime(CLOCK_REALTIME,&tv_e);
    printf("CfgClientNfs::fetch() %f s\n", time_diff(tv_b,tv_e));
#endif

    if (result > 0) {
      c.p = new char[result];
      memcpy(c.p, dst, result);
    }
    c.size = result;
  }
  return c.size;
}
