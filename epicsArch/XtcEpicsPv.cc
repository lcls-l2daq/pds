#include <memory.h>
#include "XtcEpicsPv.hh"

namespace Pds
{
Xtc* XtcEpicsPv::_pGlobalXtc = NULL;
    
int XtcEpicsPv::setValue(EpicsMonitorPv& epicsPv, bool bCtrlValue )
{      
    char* pXtcMem = (char*)(this+1);
    int iSizeXtcEpics = 0;
        
    int iFail = epicsPv.writeXtc( pXtcMem, bCtrlValue, iSizeXtcEpics );

    
    if ( iFail == 2 ) // Error code 2 means no PV connection yet, which is a special case
      return 2; // let the caller function know this case
    else if ( iFail != 0 )
      return 1;
            
    // Adjust self size
    alloc( iSizeXtcEpics );
    
    // Adjust the size of global Xtc, which contains ALL epics pv xtcs
    _pGlobalXtc->alloc( iSizeXtcEpics );
                
    return 0;
}

} // namespace Pds
