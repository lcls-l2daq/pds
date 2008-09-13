/*
** ++
** Package:
**       odfService
**
**  Abstract:
**       Abstraction of an environment variable used by the
**       dataFlow Finite-State-Machine
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**	None.
**
** --
*/

#ifndef PDS_ENV
#define PDS_ENV

namespace Pds {
class Env 
  {
  public: 
    Env() {}
    Env(const Env& in) : _env(in._env) {}
    Env(unsigned env);
    unsigned value() const;
    
    const Env& operator=(const Env& that); 
  private:  
    unsigned _env;
  };
}
/*
** ++
**
** --
*/

inline const Pds::Env& Pds::Env::operator=(const Pds::Env& that){
  _env = that._env;
  return *this;
} 

/*
** ++
**
** --
*/

inline Pds::Env::Env(unsigned env) : _env(env)
  {
  }

/*
** ++
**
** --
*/

inline unsigned Pds::Env::value() const
  {
  return _env;
  }

#endif
