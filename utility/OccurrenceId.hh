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
      Synch,
      L1Accept,
      Reserved4,   // "read event" not exported to software
      CalStrobe,
      Reserved6,   // "start playback" not exported
      Reboot,
      Gate3,
      Gate2,
      Gate2and3,
      Reserved11,  // This opcode used by "Map" in violation of BaBar note 281
      //  end of global commands...
      Reserved12,
      Reserved13,
      Reserved14,
      Reserved15,
      //  begining of software commands - i.e not forwarded to FEEs
      Svt,
      Dch,
      Drc,
      Emc,
      Ifr,
      Dct,
      Emt,
      Glt,
      OEP,
      TrimRequest,
      BkgMon,
      Calib,
      Vmon,
      Marker,
      SynchRequest,
      SequencerDone
      };
    enum {numberof = 32};
  };
}
#endif
