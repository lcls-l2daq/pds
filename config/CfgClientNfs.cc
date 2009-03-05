#include "CfgClientNfs.hh"

#include "pds/utility/Transition.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

using namespace Pds;

CfgClientNfs::CfgClientNfs( const Src& src ) :
  _src( src )
{
}

const Src& CfgClientNfs::src() const
{ return _src; }

void CfgClientNfs::initialize(const Allocate& alloc)
{
  strcpy(_path, alloc.dbpath());
}

int CfgClientNfs::fetch(const Transition& tr, 
			const TypeId&     id, 
			char*             dst)
{
  char filename[128];
  if (_src.level()==Level::Source)
    sprintf(filename,"%s/%08x/%08x/%08x",
	    _path,
	    tr.env(),
	    _src.phy(),
	    id.value());
  else
    sprintf(filename,"%s/%08x/%x/%08x",
	    _path,
	    tr.env(),
	    _src.level(),
	    id.value());

  int fd = ::open(filename,O_RDONLY);
  if (fd < 0) {
    printf("CfgClientNfs::fetch error opening %s : %s\n",
	   filename, strerror(errno));
    return 0;
  }

  int len = ::read(fd, dst, SSIZE_MAX);
  ::close(fd);

  return len;
}
