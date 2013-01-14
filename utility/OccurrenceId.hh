/*
** ++
**  Package:
**	odfUtility
**
**  Abstract:
**      
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - June 20 1,1997
**
**  Revision History:
**	001 - 98.12.14 gpdf: Defined occurrence 29 to be the "marker" type.
**
** --
*/

#ifndef PDS_OCCURRENCEID
#define PDS_OCCURRENCEID

/*
** ++
**
** --
*/

namespace Pds {
class OccurrenceId
  {
  public:
    enum Value
    {            // global commands
      Noop,        
      ClearReadout,  
      SequencerDone,
      DataFileOpened,
      UserMessage,
      EvrCommand,
      DataFileError,
      RequestPause,
      RequestConfigure,
      NumberOf
    };
  };
}
#endif
