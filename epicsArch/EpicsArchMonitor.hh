#ifndef EPICS_ARCH_MONITOR_H
#define EPICS_ARCH_MONITOR_H

#include <string>
#include <vector>
#include <set>
#include "pdsdata/epics/EpicsXtcSettings.hh"
#include "EpicsMonitorPv.hh"

namespace Pds
{
  class Pool;
  class UserMessage;

  class EpicsArchMonitor
  {
  public:
    EpicsArchMonitor(const Src & src, const std::string & sFnConfig,
      float fDefaultInterval, int iNumEventNode, Pool & occPool, int iDebugLevel);
    ~EpicsArchMonitor();
    int writeToXtc(Datagram & dg, UserMessage ** msg, const struct timespec& tsCurrent, unsigned int uVectorCur);

    static const int iXtcVersion = EpicsXtcSettings::iXtcVersion;
    static const int iMaxNumPv = EpicsXtcSettings::iMaxNumPv;
    static const int iMaxXtcSize = EpicsXtcSettings::iMaxXtcSize;
    static const TypeId::Type typeXtc = EpicsXtcSettings::typeXtc;
    static const DetInfo & detInfoEpics;

  private:
    typedef std::set    < std::string > TPvNameSet;    
  
    const Src & _src;
    std::string _sFnConfig;
    float       _fDefaultInterval;
    int         _iNumEventNode;
    Pool&       _occPool;
    int         _iDebugLevel;
    TPvNameSet  _setPv;
    TEpicsMonitorPvList _lpvPvList;    

    struct PvInfo
    {
      std::string sPvName;
      std::string sPvDescription;
      float fUpdateInterval;

        PvInfo(const std::string & sPvName1,
          const std::string & sPvDescription1,
          float fUpdateInterval1):sPvName(sPvName1),
          sPvDescription(sPvDescription1), fUpdateInterval(fUpdateInterval1)
      {
      }
    };

    typedef std::vector < PvInfo >      TPvList;
    typedef std::vector < std::string > TFileList;

    int _addPv            (const std::string & sPvList, const std::string & sPvDescription, TPvList & vPvList, bool & bPvAdd);
    int _readConfigFile   (const std::string & sFnConfig, TPvList & vPvList);    
    int _setupPvList      (const TPvList & vPvList, TEpicsMonitorPvList & lpvPvList);
    int _getPvDescription (const std::string & sLine, std::string & sPvDescription);
    
    static int _splitFileList   (const std::string & sFileList, TFileList & vsFileList);    

    // Class usage control: Value semantics is disabled
    EpicsArchMonitor(const EpicsArchMonitor &);
    EpicsArchMonitor & operator=(const EpicsArchMonitor &);

/*
 * ToDo:
 *   - Integrate iXtcVersion, srcLevel, iMaxXtcSize, typeXtc with 
 *      - pdsapp/epics/XtcEpicsMonitor.hh
 *      - pdsdata/app/XtcEpicsIterator.hh
 */
  };



}       // namespace Pds

#endif
