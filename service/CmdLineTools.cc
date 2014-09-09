#include "pds/service/CmdLineTools.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

using namespace Pds;

bool CmdLineTools::parseDetInfo(const char* args, DetInfo& info)
{
  DetInfo::Detector det(DetInfo::NumDetector);
  DetInfo::Device   dev(DetInfo::NumDevice);
  unsigned detid(0), devid(0);

  printf("Parsing %s\n",args);

  char* p;
  det    = (DetInfo::Detector)strtoul(args, &p, 0);
  if (p != args) {
    detid  = strtoul(p+1 , &p, 0);
    dev    = (DetInfo::Device)strtoul(p+1 , &p, 0);
    devid  = strtoul(p+1 , &p, 0);
  }
  else {
    int n = (p=const_cast<char*>(strchr(args,'/'))) - args;
    det = DetInfo::NumDetector;
    for(int i=0; i<DetInfo::NumDetector; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Detector)i),n)==0) {
        det = (DetInfo::Detector)i;
        break;
      }
    if (det == DetInfo::NumDetector) {
      printf("Parsing detector %s failed\n", args);
      printf("Choices are:");
      for(int i=0; i<DetInfo::NumDetector; i++)
        printf(" %s",DetInfo::name((DetInfo::Detector)i));
      printf("\n");
      return false;
    }

    detid  = strtoul(p+1 , &p, 0);

    args = p+1;
    n = (p=const_cast<char*>(strchr(args,'/'))) - args;
    dev = DetInfo::NumDevice;
    for(int i=0; i<DetInfo::NumDevice; i++)
      if (strncasecmp(args,DetInfo::name((DetInfo::Device)i),n)==0) {
        dev = (DetInfo::Device)i;
        break;
      }
    if (dev == DetInfo::NumDevice) {
      printf("Parsing device %s failed\n", args);
      printf("Choices are:");
      for(int i=0; i<DetInfo::NumDevice; i++)
        printf(" %s",DetInfo::name((DetInfo::Device)i));
      printf("\n");
      return false;
    }

    devid  = strtoul(p+1 , &p, 0);
  }

  info = DetInfo(0, det, detid, dev, devid);
  printf("Sourcing %s\n",DetInfo::name(info));
  return true;
}


bool CmdLineTools::parseInt   (const char* arg, int& v, int base)
{
  char* endptr;
  v = strtol(arg,&endptr,base);
  return *endptr==0;
}

bool CmdLineTools::parseUInt  (const char* arg, unsigned& v, int base)
{
  char* endptr;
  v = strtoul(arg,&endptr,base);
  return *endptr==0;
}

int CmdLineTools::parseUInt  (char* arg, unsigned& v1, unsigned& v2, unsigned& v3, int base, int delim)
{
  int count = 0;
  char* endptr;
  char* nextptr;

  nextptr = index(arg, delim);        // find 1st delim
  if (nextptr) {
    *nextptr++ = '\0';                // replace 1st delim with NULL
  }
  v1 = strtoul(arg,&endptr,base);
  if (*endptr==0) {
    ++count;
    if (nextptr) {
      arg = nextptr;
      nextptr = index(arg, delim);    // find 2nd delim
      if (nextptr) {
        *nextptr++ = '\0';            // replace 2nd delim with NULL
      }
      v2 = strtoul(arg,&endptr,base);
      if (*endptr==0) {
        ++count;
        if (nextptr) {
          v3 = strtoul(nextptr,&endptr,base);
          if (*endptr==0) {
            ++count;
          } else {
            count = 0;                // error
          }
        }
      } else {
        count = 0;                    // error
      }
    }
  } else {
    count = 0;                        // error
  }

  return count;
}

bool CmdLineTools::parseUInt64(const char* arg, uint64_t& v, int base)
{
  char* endptr;
  v = strtoull(arg,&endptr,base);
  return *endptr==0;
}

bool CmdLineTools::parseFloat (const char* arg, float& v)
{
  char* endptr;
  v = strtof(arg,&endptr);
  return *endptr==0;
}

bool CmdLineTools::parseDouble(const char* arg, double& v)
{
  char* endptr;
  v = strtod(arg,&endptr);
  return *endptr==0;
}

bool CmdLineTools::parseSrcAlias(const char* arg)
{
  static char prohibited[] = "/|\\:, \f\n\r\t\v";
  char *pp;

  if ((!arg) || (!*arg) || (strlen(arg) > 30)) {
    return false;       // too long or too short
  }
  for (pp = prohibited; *pp; pp++) {
    if (index(arg, *pp)) {
      return false;     // prohibited char
    }
  }
  return true;
}
