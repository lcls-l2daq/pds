#include "pds/lecroy/ConfigServer.hh"
#include "pds/lecroy/Server.hh"
#include <stdio.h>
#include <stdint.h>

using namespace Pds::LeCroy;

ConfigServer::ConfigServer(const char* name) :
  Pds_Epics::EpicsCA (name,this),
  _srv(0)
{
}

ConfigServer::ConfigServer(const char* name, Server* srv) :
  Pds_Epics::EpicsCA (name,this),
  _srv(srv)
{
}

ConfigServer::~ConfigServer()
{
}

void ConfigServer::updated() 
{
  printf("ConfigServer[%s] updated\n",_channel.epicsName());
  if (_srv) _srv->signal();
}

#define handle_type(ctype, stype, dtype) case ctype:    \
  { dtype* inp  = (dtype*)data();                       \
    dtype* outp = (dtype*)payload;                      \
    for(int k=0; k<nelem; k++) *outp++ = *inp++;        \
    result = (char*)outp - (char*)payload;              \
  }


int ConfigServer::fetch(char* payload)
{
#ifdef DBUG
  printf("ConfigServer[%s] fetch %p\n",_channel.epicsName(),payload);
#endif
  int result = 0;
  int nelem = _channel.nelements();
  switch(_channel.type()) {
    handle_type(DBR_TIME_SHORT , dbr_time_short , dbr_short_t ) break;
    handle_type(DBR_TIME_FLOAT , dbr_time_float , dbr_float_t ) break;
    handle_type(DBR_TIME_ENUM  , dbr_time_enum  , dbr_enum_t  ) break;
    handle_type(DBR_TIME_LONG  , dbr_time_long  , dbr_long_t  ) break;
    handle_type(DBR_TIME_DOUBLE, dbr_time_double, dbr_double_t) break;
    handle_type(DBR_TIME_CHAR  , dbr_time_char  , dbr_char_t  ) break;
  default: printf("Unknown type %d\n", int(_channel.type())); result=-1; break;
  }
  return result;
}

void ConfigServer::update() { _channel.get(); }
