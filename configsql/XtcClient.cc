#include "pds/configsql/XtcClient.hh"
#include "pds/configsql/DbClient.hh"
#include "pds/config/DeviceEntry.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

using namespace Pds_ConfigDb::Sql;

XtcClient::XtcClient(const char* path) : _db(DbClient::open(path))
{
  printf("Sql::XtcClient %s\n",path);
}

XtcClient::~XtcClient()
{
  delete _db;
}

XtcClient* XtcClient::open  ( const char* path ) 
{
  return new XtcClient(path); 
}

int       XtcClient::getXTC( unsigned           key,
                             const Pds::Src&    src,
                             const Pds::TypeId& type_id,
                             void*              dst,
                             unsigned           maxSize)
{
  std::list<KeyEntry> klist = _db->getKey(key);
  for(std::list<KeyEntry>::iterator it=klist.begin();
      it!=klist.end(); it++) {
    DeviceEntry d(it->source);
    if (d.level() == src.level() &&
        (d.phy  () == src.phy() ||
         d.level() != Pds::Level::Source) &&
        it->xtc.type_id.value() == type_id.value())
      return _db->getXTC(it->xtc, dst, maxSize);
  }

  return 0;
}
