#ifndef EPICS_ARCH_MONITOR_H
#define EPICS_ARCH_MONITOR_H

#include <string>
#include <vector>
#include "pdsdata/epics/EpicsXtcSettings.hh"
#include "EpicsMonitorPv.hh"

namespace Pds
{
    
class EpicsArchMonitor
{
public:
    EpicsArchMonitor( const std::string& sFnConfig, int iDebugLevel );
    ~EpicsArchMonitor();
    int writeToXtc( Datagram& dg, bool bCtrlValue );
    
    static const int            iXtcVersion = EpicsXtcSettings::iXtcVersion;    
    static const int            iMaxXtcSize = EpicsXtcSettings::iMaxXtcSize;
    static const TypeId::Type   typeXtc     = EpicsXtcSettings::typeXtc;
    static const DetInfo&        detInfoEpics;
    
private:        

    std::string         _sFnConfig;
    int                 _iDebugLevel;
    TEpicsMonitorPvList _lpvPvList;     
    
    typedef std::vector<std::string> TPvList;
    
    static int _readConfigFile( const std::string& sFnConfig, TPvList& vsPvNameList );
    static int _setupPvList( const TPvList& vsPvList, TEpicsMonitorPvList& lpvPvList);
    static int _splitPvList( const std::string& sPvList, TPvList& vsPv );
    
    static const char sPvListSeparators[];
    
    // Class usage control: Value semantics is disabled
    EpicsArchMonitor( const EpicsArchMonitor& );
    EpicsArchMonitor& operator=(const EpicsArchMonitor& );    
    
/*
 * ToDo:
 *   - Integrate iXtcVersion, srcLevel, iMaxXtcSize, typeXtc with 
 *      - pdsapp/epics/XtcEpicsMonitor.hh
 *      - pdsdata/app/XtcEpicsIterator.hh
 */    
};

 

} // namespace Pds

#endif
