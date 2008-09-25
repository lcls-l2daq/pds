/*
** ++
**  Package:
**	OdfCommon
**
**  Abstract:
**      odfTransitionId is a dataless class defining a class-local enum 
**      over the valid online state diagram transitions.
**
**      It is in a common online/offline package because the transitions'
**      identities are useful information offline.
**      
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revisor initials:
**      gpdf - Gregory Dubois-Felsmann
**
**  Revision History:
**	001 - gpdf 1997.12.11
**            File moved from Odf/utility to OdfCommon.  Enum definition
**            changed to use explicit assignment of enumeration values.
**
** --
*/

#ifndef PDS_EVENTID
#define PDS_EVENTID

/*
** ++
**
**   WARNING!  The numerical values corresponding to particular events
**   MUST NEVER CHANGE during the lifetime of the experiment, as they will
**   be stored in the persistent data.
**
**   If events are deleted from the model, their numbers must never be
**   reused, and if new ones are added, they must be added at the end of the
**   list with higher values.
**
** --
*/

namespace Pds {
  class EventId
  {
  public:
    enum Value
      {
	Map,       Unmap,     Configure, Unconfigure,
	BeginRun,  EndRun,    Pause,     Resume,
	Enable,    Disable,   L1Accept
      };
    
    /*
    ** The "numberof" constant must always be one more than the highest
    ** numerical value of a event!
    */
    enum { numberof = 11 };
  };
};

#endif
