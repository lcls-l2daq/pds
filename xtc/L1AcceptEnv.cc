#include "pds/xtc/L1AcceptEnv.hh"

using namespace Pds;

//static const unsigned event_bit  = (1UL<<31);
//static const unsigned event_mask = (1UL<<31)-1;

L1AcceptEnv::L1AcceptEnv() : Env(0) {}
L1AcceptEnv::L1AcceptEnv(unsigned n) : Env(n) {}
L1AcceptEnv::L1AcceptEnv(Env& env) : Env(env) {}


uint32_t L1AcceptEnv::clientGroupMask()
{
  return value();
}
