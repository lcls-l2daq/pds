#include "pds/camera/AdimecCommander.hh"
#include "pds/camera/CameraDriver.hh"

#include <sstream>
#include <string.h>
#include <stdio.h>
#include <errno.h>

using namespace Pds;

AdimecCommander::AdimecCommander(CameraDriver& _driver) : driver(_driver) 
{
  memset(szCommand ,0,SZCOMMAND_MAXLEN);
  memset(szResponse,0,SZCOMMAND_MAXLEN);
}

int AdimecCommander::setCommand(const char* title,
                                const char* cmd) {
  int ret;
  snprintf(szCommand, SZCOMMAND_MAXLEN, cmd);
  if ((ret = driver.SendCommand(szCommand, NULL, 0))<0)
    printf("Error on command %s (%s)\n",cmd,title);
  return ret;
}

void AdimecCommander::getParameter(const char* cmd, 
                                   unsigned& val1) {
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd);
  driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN);
  sscanf(szResponse+2,"%u",&val1);
}

void AdimecCommander::getParameters(const char* cmd,
                                    unsigned* valn,
                                    unsigned n) {
  snprintf(szCommand, SZCOMMAND_MAXLEN, "%s?", cmd);
  driver.SendCommand(szCommand, szResponse, SZCOMMAND_MAXLEN);
  std::stringstream ss(szResponse+2);
  ss >> valn[0];
  for(unsigned ii=1; ii<n; ii++) {
    ss.get();
    ss >> valn[ii];
  }
}

int AdimecCommander::setParameter(const char* title,
                                  const char* cmd,
                                  unsigned    val1) {
  unsigned nretry = 1;
  unsigned rval;
  unsigned val = val1;
  do {
    getParameter(cmd,rval);
    printf("Read %s = d%u\n", title, rval);
    printf("Setting %s = d%u\n",title,val);
    snprintf(szCommand, SZCOMMAND_MAXLEN, "%s%u", cmd, val);
    int ret = driver.SendCommand(szCommand, NULL, 0);
    if (ret<0) return ret;
    getParameter(cmd,rval);
    printf("Read %s = d%u\n", title, rval);
  } while ( nretry-- && (rval != val) );
  if (rval != val) return -EINVAL;
  return 0;
}

int AdimecCommander::setParameter(const char* title,
                                  const char* cmd,
                                  unsigned* valn,
                                  unsigned n) {
  unsigned nretry = 1;
  unsigned rvaln[n];
  bool lv=true;
  do {
    printf("Setting %s = ",title);
    for(unsigned ii=0; ii<n; ii++)
      printf("d%u%c",valn[ii],(ii<(n-1))?';':'\n');
    char* pp = szCommand;
    pp += snprintf(pp, pp+SZCOMMAND_MAXLEN-szCommand, "%s%u",cmd,valn[0]);
    for(unsigned ii=1; ii<n; ii++)
      pp += snprintf(pp,pp+SZCOMMAND_MAXLEN-szCommand, ";%u",valn[ii]);
    int ret = driver.SendCommand(szCommand, NULL, 0);
    if (ret<0) return ret;
    getParameters(cmd,rvaln,n);
    printf("Read %s = ",title);
    for(unsigned ii=0; ii<n; ii++)
      printf("d%u%c",rvaln[ii],ii<(n-1)?';':'\n');
    lv=true;
    for(unsigned ii=0; ii<n; ii++)
      lv &= (rvaln[ii]==valn[ii]);
  } while( nretry-- && !lv);
  if (!lv) return -EINVAL;
  for(unsigned ii=0; ii<n; ii++)
    valn[ii] = rvaln[ii];
  return 0;
}

int AdimecCommander::setParameter(const char* cmd,
                                  const ndarray<const uint16_t,1>& a) {
  int ret;
  for(unsigned k=0; k<a.shape()[0]; k++) {
    snprintf(szCommand, SZCOMMAND_MAXLEN, cmd, a[k]);
    if ((ret = driver.SendCommand(szCommand, NULL, 0))<0)
      return ret;
  }
  return 0;
}
