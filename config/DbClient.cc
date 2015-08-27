#include "pds/confignfs/DbClient.hh"
#include "pds/configsql/DbClient.hh"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace Pds_ConfigDb;

DbClient* DbClient::open(const char* path)
{                                               
  struct stat64 s;

  char buff[512];
  snprintf(buff, 512, path);
  buff[511]=0;

  do {
    if (stat64(buff,&s)) {
      perror("configdb path not found");
      return 0;
    }

    if (S_ISDIR(s.st_mode)) 
      return Nfs::DbClient::open(buff);

    if (S_ISREG(s.st_mode))
      return Sql::DbClient::open(buff);

    if (S_ISLNK(s.st_mode) &&
        readlink(path,buff,512)>=0) {
      continue;
    }
    else
      break;

  } while (1);

  printf("configdb path type unknown [%s]\n",buff);
  return 0;
}
