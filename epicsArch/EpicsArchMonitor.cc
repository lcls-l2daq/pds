#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"
#include "pdsdata/xtc/TypeId.hh"
#include "XtcEpicsPv.hh"
#include "EpicsArchMonitor.hh"

namespace Pds
{
    
using std::string;

const DetInfo& EpicsArchMonitor::detInfoEpics = EpicsXtcSettings::detInfo;
const char EpicsArchMonitor::sPvListSeparators[] = " ,;\t\r\n";

EpicsArchMonitor::EpicsArchMonitor( const std::string& sFnConfig, int iDebugLevel ) :
  _sFnConfig(sFnConfig), _iDebugLevel(iDebugLevel)
{
    if ( _sFnConfig == "" )
        throw string("EpicsArchMonitor::EpicsArchMonitor(): Invalid parameters");
        
    TPvList vsPvNameList;
    int iFail = _readConfigFile( sFnConfig, vsPvNameList );
    if ( iFail != 0 )
        throw "EpicsArchMonitor::EpicsArchMonitor()::readConfigFile(" + _sFnConfig + ") failed\n";
        
    if ( vsPvNameList.size() == 0 )
        throw "EpicsArchMonitor::EpicsArchMonitor(): No Pv Name is specified in the config file " + _sFnConfig;                
       
    // initialize Channel Access        
    if (ECA_NORMAL != ca_task_initialize()) 
    {
        throw "EpicsArchMonitor::EpicsArchMonitor(): ca_task_initialize() failed" + _sFnConfig;                
    };
        
    printf( "Monitoring Pv: " );
    for ( int iPv = 0; iPv < (int) vsPvNameList.size(); iPv++ )
    {
        if ( iPv != 0 ) printf( ", " );
        printf( "[%d] %s", iPv, vsPvNameList[iPv].c_str() );
    }
    printf( "\n" );
        
    iFail = _setupPvList( vsPvNameList, _lpvPvList );
    if (iFail != 0)
        throw string("EpicsArchMonitor::EpicsArchMonitor()::setupPvList() Failed");        
}

EpicsArchMonitor::~EpicsArchMonitor()
{    
    for ( int iPvName = 0; iPvName < (int) _lpvPvList.size(); iPvName++ )
    {           
        EpicsMonitorPv& epicsPvCur = _lpvPvList[iPvName];
        int iFail = epicsPvCur.release();
        
        if (iFail != 0)
            printf( "xtcEpicsTest()::EpicsMonitorPv::release(%s) failed\n", epicsPvCur.getPvName().c_str());
    }    
    
    int iFail = ca_task_exit();
    if (ECA_NORMAL != iFail ) 
      SEVCHK( iFail, "EpicsArchMonitor::~EpicsArchMonitor(): ca_task_exit() failed" );    
}

int EpicsArchMonitor::writeToXtc( Datagram& dg, bool bCtrlValue )
{
    const int iNumPv = _lpvPvList.size();
    
    ca_poll();
    
    TypeId typeIdXtc(EpicsArchMonitor::typeXtc, EpicsArchMonitor::iXtcVersion);
    char* pPoolBufferOverflowWarning = (char*) &dg + (int)(0.9*EpicsArchMonitor::iMaxXtcSize);    
    
    bool bAnyPvWriteOkay      = false;
    bool bSomePvWriteError    = false; 
    bool bSomePvNotConnected  = false; // PV not connected: Less serious than the "Write Error"
    for ( int iPvName = 0; iPvName < iNumPv; iPvName++ )
    {
        EpicsMonitorPv& epicsPvCur = _lpvPvList[iPvName];
              
        if (_iDebugLevel >= 1 ) epicsPvCur.printPv();
        
        XtcEpicsPv* pXtcEpicsPvCur = new(&dg.xtc) XtcEpicsPv(typeIdXtc, detInfoEpics);

        int iFail = pXtcEpicsPvCur->setValue( epicsPvCur, bCtrlValue );
        if ( iFail == 0 ) 
          bAnyPvWriteOkay = true;
        else if ( iFail == 2 ) // Error code 2 means this PV has no connection
        {
          if ( bCtrlValue ) // If this happens in the "config" action (ctrl value is about to be written)
            epicsPvCur.release(); // release this PV and never update it again
            
          bSomePvNotConnected = true;
        }
        else
          bSomePvWriteError = true;
            
        char* pCurBufferPointer = (char*) dg.xtc.next();                
        if ( pCurBufferPointer > pPoolBufferOverflowWarning  )
        {
            printf( "EpicsArchMonitor::writeToXtc(): Pool buffer size is too small.\n" );
            printf( "EpicsArchMonitor::writeToXtc(): %d Pvs are stored in 90%% of the pool buffer (size = %d bytes)\n", 
              iPvName+1, EpicsArchMonitor::iMaxXtcSize );              
            return 2;
        }
    }
    
    /// ca_pend For debug print
    //printf("Size of Data: Xtc %d Datagram %d Payload %d Total %d\n", 
    //    sizeof(Xtc), sizeof(Datagram), pDatagram->xtc.sizeofPayload(), 
    //    sizeof(Datagram) + pDatagram->xtc.sizeofPayload() );

    // checking errors, from the most serious one to the less serious one
    if ( !bAnyPvWriteOkay )     return 3; // No PV has been outputted    
    if ( bSomePvWriteError )    return 4; // Some PV values have been outputted, but some has write error    
    if ( bSomePvNotConnected )  return 5; // Some PV values have been outputted, but some has not been connected

    return 0; // All PV values are outputted successfully
}

/*
 * private static member functions
 */

int EpicsArchMonitor::_readConfigFile( const std::string& sFnConfig, TPvList& vsPvNameList )
{
        
    std::ifstream ifsConfig( sFnConfig.c_str() );    
    if ( !ifsConfig ) return 1; // Cannot open file
    
    std::string sFnPath;
    size_t uOffsetPathEnd = sFnConfig.find_last_of( '/' );
    if ( uOffsetPathEnd != string::npos )      
      sFnPath.assign( sFnConfig, 0, uOffsetPathEnd+1 );
      
    int iLineNumber = 0;
    
    while ( !ifsConfig.eof() )
    {
        ++iLineNumber;
        string sLine;        
        std::getline( ifsConfig, sLine );    
        if ( sLine[0] == '#' ) continue; // skip comment lines that begin with '#'        
        if ( sLine[0] == '<' ) 
        {
          sLine[0] = ' ';
          TPvList vsPvFileLst;
          _splitPvList( sLine, vsPvFileLst );
          
          for ( int iPvFile = 0; iPvFile < (int) vsPvFileLst.size(); iPvFile++ )
          {
            string sFnRef = vsPvFileLst[iPvFile];
            if ( sFnRef[0] != '/' )
              sFnRef = sFnPath + sFnRef;
            int iFail     = _readConfigFile( sFnRef, vsPvNameList );
            if ( iFail != 0 )
            {
              printf( "EpicsArchMonitor::_readConfigFile(): Invalid file reference \"%s\", in file \"%s\":line %d\n",
                sFnRef.c_str(), sFnConfig.c_str(), iLineNumber );                
            }
          }
          continue;
        }
        
        _splitPvList( sLine, vsPvNameList );
    }
        
    return 0;
}

int EpicsArchMonitor::_setupPvList(const TPvList& vsPvList, TEpicsMonitorPvList& lpvPvList)
{   
    if ( vsPvList.size() == 0 ) 
      return 0;
    if ( (int) vsPvList.size() >= iMaxNumPv )
      printf( "EpicsArchMonitor::_setupPvList(): Number of PVs (%d) has reached the system capacity (%d), "
        "some PVs in the list may have been skipped.\n",
        vsPvList.size(), iMaxNumPv );
        
        
    lpvPvList.resize(vsPvList.size());
    for ( int iPvName = 0; iPvName < (int) vsPvList.size(); iPvName++ )
    {                
        EpicsMonitorPv& epicsPvCur = lpvPvList[iPvName];
        
        string sPvName( vsPvList[iPvName] );
        int iFail = epicsPvCur.init( iPvName, sPvName.c_str() );
        
        if (iFail != 0)
        {
            printf( "EpicsArchMonitor::setupPvList()::EpicsMonitorPv::init(%s) failed\n", epicsPvCur.getPvName().c_str());
            return 1;
        }       
    }    
    
    /* flush all CA monitor requests*/
    ca_flush_io();
    ca_pend_event(0.5); // empirically, 0.5s is enough for 1000 PVs to be updated 
    return 0;
}

int EpicsArchMonitor::_splitPvList( const string& sPvList, TPvList& vsPvList )
{       
    size_t uOffsetStart = sPvList.find_first_not_of( EpicsArchMonitor::sPvListSeparators, 0 );
    while ( uOffsetStart != string::npos )      
    {     
        if ( sPvList[uOffsetStart] == '#' ) break; // skip the remaining characters
      
        size_t uOffsetEnd = sPvList.find_first_of( EpicsArchMonitor::sPvListSeparators, uOffsetStart+1 );
        
        if ( uOffsetEnd == string::npos )        
        {
            if ( (int) vsPvList.size() < iMaxNumPv )
              vsPvList.push_back( sPvList.substr( uOffsetStart, string::npos ) );
              
            break;
        }
        
        if ( (int) vsPvList.size() < iMaxNumPv )
          vsPvList.push_back( sPvList.substr( uOffsetStart, uOffsetEnd - uOffsetStart ) );
        else
          break;
                  
        uOffsetStart = sPvList.find_first_not_of( sPvListSeparators, uOffsetEnd+1 );        
    }
    return 0;
}

} // namespace Pds
