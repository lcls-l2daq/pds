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

//#define RECONNECT

using namespace Pds;

#ifdef DBUG
static inline double time_diff(struct timespec& b, 
                               struct timespec& e)
{
  return double(e.tv_sec - b.tv_sec) + 1.e-9*
    (double(e.tv_nsec) - double(b.tv_nsec));
}
#endif

CfgClientNfs::CfgClientNfs( const Src& src ) :
  _src( src ),
  _db ( 0 )
{
}

CfgClientNfs::~CfgClientNfs()
{
#ifndef RECONNECT
  if (_db) delete _db;
#endif
}

const Src& CfgClientNfs::src() const
{ return _src; }

void CfgClientNfs::initialize(const Allocation& alloc)
{
#ifndef RECONNECT  
  if (_db) delete _db;

  _db = Pds_ConfigDb::XtcClient::open(alloc.dbpath());
#else
  _path = std::string(alloc.dbpath());
#endif
}

int CfgClientNfs::fetch(const Transition& tr, 
			const TypeId&     id, 
			void*             dst,
			unsigned          maxSize)
{
#ifdef DBUG
  struct timespec tv_b;
  clock_gettime(CLOCK_REALTIME,&tv_b);
#endif

#ifndef RECONNECT
  Pds_ConfigDb::XtcClient* db = _db;
#else
  Pds_ConfigDb::XtcClient* db = Pds_ConfigDb::XtcClient::open(_path.c_str());
#endif

  int result = db->getXTC(tr.env().value(),
                          _src,
                          id,
                          dst,
                          maxSize);

#ifdef RECONNECT
  delete db;
#endif

#ifdef DBUG
  struct timespec tv_e;
  clock_gettime(CLOCK_REALTIME,&tv_e);
  printf("CfgClientNfs::fetch() %f s\n", time_diff(tv_b,tv_e));
#endif
  return result;
}
