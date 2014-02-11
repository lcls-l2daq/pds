#include "pds/confignfs/XtcClient.hh"
#include "pds/configsql/XtcClient.hh"

#include <sys/stat.h>

using namespace Pds_ConfigDb;

XtcClient* XtcClient::open(const char* path)
{                                               
  struct stat64 s;
  if (stat64(path,&s)) {
    perror("configdb path not found");
    return 0;
  }

  if (S_ISDIR(s.st_mode))
    return Nfs::XtcClient::open(path);

  else
    return Sql::XtcClient::open(path);
}
