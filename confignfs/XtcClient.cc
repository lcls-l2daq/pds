#include "pds/confignfs/XtcClient.hh"
#include "pds/confignfs/CfgPath.hh"
#include "pdsdata/xtc/Src.hh"
#include "pdsdata/xtc/TypeId.hh"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds_ConfigDb::Nfs;

XtcClient::XtcClient(const char* path) : _path(path) {}

XtcClient::~XtcClient() {}

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
  char filename[128];
  sprintf(filename,"%s/keys/%s",
          _path.c_str(),
          CfgPath::path(key,src,type_id).c_str());
  int fd = ::open(filename,O_RDONLY);
  if (fd < 0) {
    printf("XtcClient::fetch error opening %s : %s\n",
	   filename, strerror(errno));
    return 0;
  }

  int len = ::read(fd, dst, maxSize);
  ::close(fd);

  return len;
}
