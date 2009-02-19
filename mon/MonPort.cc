#include "pds/mon/MonPort.hh"
#include <string.h>

using namespace Pds;

unsigned short MonPort::port(Type type)
{
  const unsigned short Port = 5713;
  return Port+type;
}

const char* MonPort::name(Type type)
{
  switch (type) {
  case Mon:
    return "Mon";
  case Vmon:
    return "Vmon";
  case Dhp:
    return "Dhp";
  case Vtx:
    return "Vtx";
  case Fct:
    return "Fct";
  case Test:
    return "Test";
  default:
    return 0;
  }
}

MonPort::Type MonPort::type(const char* name)
{
  if(      strcmp(name,"Mon")==0 )
    return MonPort::Mon;
  else if( strcmp(name,"Vmon")==0 )
    return MonPort::Vmon;
  else if( strcmp(name,"Dhp")==0 )
    return MonPort::Dhp;
  else if( strcmp(name,"Vtx")==0 )
    return MonPort::Vtx;
  else if( strcmp(name,"Fct")==0 )
    return MonPort::Fct;
  else if( strcmp(name,"Test")==0 )
    return MonPort::Test;
  return MonPort::NTypes;
}

