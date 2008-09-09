#include <stdio.h>

#include "Ether.hh"

using namespace Pds;


Ether::Ether()
{
  _mac[0] = 0;
  _mac[1] = 0;
}

Ether::Ether(const Ether& ether)
{
  _mac[0] = ether._mac[0];
  _mac[1] = ether._mac[1];
}

Ether::Ether(const unsigned char* as_array)
{
  _mac[0] = (as_array[0]<<24)|(as_array[1]<<16)|(as_array[2]<<8)|as_array[3];
  _mac[1] = (as_array[4]<<24)|(as_array[5]<<16);
}

const unsigned char* Ether::as_array(unsigned char* buffer) const
{
  buffer[0] = (_mac[0]>>24)&0xff;
  buffer[1] = (_mac[0]>>16)&0xff;
  buffer[2] = (_mac[0]>> 8)&0xff;
  buffer[3] = (_mac[0]    )&0xff;
  buffer[4] = (_mac[1]>>24)&0xff;
  buffer[5] = (_mac[1]>>16)&0xff;
  return buffer;
}

const char* Ether::as_string(char* buffer) const
{
  sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", 
	  (_mac[0]>>24)&0xff,
	  (_mac[0]>>16)&0xff,
	  (_mac[0]>> 8)&0xff,
	  (_mac[0]    )&0xff,
	  (_mac[1]>>24)&0xff,
	  (_mac[1]>>16)&0xff);
  return buffer;
}
