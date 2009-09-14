#ifndef EPICS_ARCH_MONITOR_H
#define EPICS_ARCH_MONITOR_H

#include <string>
#include <vector>
#include "pdsdata/xtc/TypeId.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Src.hh"
#include "EpicsMonitorPv.hh"

namespace Pds
{
    
class EpicsArchMonitor
{
public:
    EpicsArchMonitor( const std::string& sFnConfig, int iDebugLevel );
    ~EpicsArchMonitor();
    int writeToXtc( Datagram& dg );
    
    static const int            iXtcVersion = 1;    
    static const int            iMaxXtcSize = 360100; // Space enough for 2000 PVs of type DBR_DOUBLE
    static const TypeId::Type   typeXtc     = TypeId::Id_Epics;
    static const Src            srcLevel;
    
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
