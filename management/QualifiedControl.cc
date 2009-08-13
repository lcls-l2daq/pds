#include "QualifiedControl.hh"

using namespace Pds;


QualifiedControl::QualifiedControl(unsigned         platform,
				   ControlCallback& cb,
				   Routine*         tmo,
				   Arp*             arp) :
  PartitionControl(platform, cb, tmo, arp),
  _unqualified_target_state(PartitionControl::Unmapped)
{
  for(unsigned k=0; k<PartitionControl::NumberOfStates; k++)
    _enabled[k] = true;
}

void QualifiedControl::set_target_state(PartitionControl::State target)
{
  _unqualified_target_state    = target;
  int next = int(PartitionControl::Unmapped);
  while( next < target && _enabled[next+1] )
    next++;
  
  PartitionControl::set_target_state(PartitionControl::State(next));
}

void QualifiedControl::enable (PartitionControl::State s, bool e) 
{
  _enabled[s] = e;
  set_target_state(_unqualified_target_state);
}

bool QualifiedControl::enabled(PartitionControl::State s) const { return _enabled[s]; }
