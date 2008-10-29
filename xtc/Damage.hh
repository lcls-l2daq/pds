#ifndef Pds_Damage_hh
#define Pds_Damage_hh

#include "pds/service/ODMGTypes.hh"

namespace Pds {

  class Damage {
  public:
    enum Value {
      DroppedContribution    = 1,
      UserDefined            = 14,
      IncompleteContribution = 15
    };
    Damage(unsigned v) : _damage(v) {}
    d_ULong  value() const             { return _damage; }
    void     increase(Damage::Value v) { _damage |= (1<<v); }
    void     increase(d_ULong v)       { _damage |= v; }
    
  private:
    d_ULong _damage;
  };
}

#endif
