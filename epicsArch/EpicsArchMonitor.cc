#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "pdsdata/xtc/TypeId.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/config/EpicsConfigType.hh"
#include "XtcEpicsPv.hh"
#include "EpicsArchMonitor.hh"

namespace Pds
{

using std::string;
using std::stringstream;

EpicsArchMonitor::EpicsArchMonitor(const Src & src, const std::string & sFnConfig,
  float fDefaultInterval, int iNumEventNode, Pool & occPool, int iDebugLevel, std::string& sConfigFileWarning):
  _src(src), _sFnConfig(sFnConfig), _fDefaultInterval(fDefaultInterval), _iNumEventNode(iNumEventNode),
  _occPool(occPool), _iDebugLevel(iDebugLevel)
{
  if (_sFnConfig == "")
    throw string("EpicsArchMonitor::EpicsArchMonitor(): Invalid parameters");

  Pds::PvConfigFile::TPvList vPvList;
  int iMaxDepth = 10;
  PvConfigFile configFile(_sFnConfig, _fDefaultInterval, iMaxDepth, iMaxNumPv, (_iDebugLevel >= 1));
  int iFail = configFile.read(vPvList, sConfigFileWarning);
  if (iFail != 0)
      throw "EpicsArchMonitor::EpicsArchMonitor(): configFile(" + _sFnConfig + ").read() failed\n";

  if (vPvList.empty())
    throw
      "EpicsArchMonitor::EpicsArchMonitor(): No Pv Name is specified in the config file "
      + _sFnConfig;

  // initialize Channel Access
  if (ECA_NORMAL != ca_task_initialize())
  {
    throw
      "EpicsArchMonitor::EpicsArchMonitor(): ca_task_initialize() failed" +
      _sFnConfig;
  };

  printf("Monitoring Pv:\n");
  for (int iPv = 0; iPv < (int) vPvList.size(); iPv++)
    printf("  [%d] %-32s PV %-32s Interval %g s\n", iPv,
    vPvList[iPv].sPvDescription.c_str(), vPvList[iPv].sPvName.c_str(),
    vPvList[iPv].fUpdateInterval);
  printf("\n");

  iFail = _setupPvList(vPvList, _lpvPvList);
  if (iFail != 0)
    throw string("EpicsArchMonitor::EpicsArchMonitor()::setupPvList() Failed");
}

EpicsArchMonitor::~EpicsArchMonitor()
{
  for (int iPvName = 0; iPvName < (int) _lpvPvList.size(); iPvName++)
  {
    EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];
    int iFail = epicsPvCur.release();

    if (iFail != 0)
      printf("xtcEpicsTest()::EpicsMonitorPv::release(%s (%s)) failed\n",
             epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());
  }

  int iFail = ca_task_exit();
  if (ECA_NORMAL != iFail)
    SEVCHK(iFail,
           "EpicsArchMonitor::~EpicsArchMonitor(): ca_task_exit() failed");
}

//static int getLocalTime( const timespec& ts, char* sTime )
//{
//  static const char timeFormatStr[40] = "%04Y-%02m-%02d %02H:%02M:%02S"; /* Time format string */
//  static char sTimeText[64];

//  struct tm tmTimeStamp;
//  time_t    tOrg = ts.tv_sec;
//  localtime_r( (const time_t*) (void*) &tOrg, &tmTimeStamp );
//  strftime(sTimeText, sizeof(sTimeText), timeFormatStr, &tmTimeStamp );

//  strncpy(sTime, sTimeText, sizeof(sTimeText));
//  return 0;
//}

int EpicsArchMonitor::writeToXtc(Datagram & dg, UserMessage ** msg, const struct timespec& tsCurrent, unsigned int uVectorCur)
{
  bool bCtrlValue = (msg != 0);

  const int iNumPv = _lpvPvList.size();

  if (!bCtrlValue)
  {
    bool bAnyPvWantsWrite = false;

    for (int iPvName = 0; iPvName < iNumPv; iPvName++)
    {
      EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];

      if (epicsPvCur.checkWriteEvent(tsCurrent, uVectorCur) )
        bAnyPvWantsWrite = true;
    }

    if (!bAnyPvWantsWrite)
      return 0;
  }
  else
  {
    Xtc*              pXtcConfig  = new (&dg.xtc) Xtc(_epicsConfigType, _src);

    new (pXtcConfig->alloc(sizeof(EpicsConfigType)) ) EpicsConfigType(iNumPv);

    for (int iPvName = 0; iPvName < iNumPv; iPvName++)
    {
      EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];
      new  (pXtcConfig->alloc(sizeof(Pds::EpicsConfig::PvConfigType)) )
        Pds::EpicsConfig::PvConfigType( epicsPvCur.getPvId(), epicsPvCur.getPvDescription().c_str(),
          epicsPvCur.getUpdateInterval() );
    }

    dg.xtc.alloc(pXtcConfig->sizeofPayload());
  }

  ca_poll();

  TypeId typeIdXtc(EpicsArchMonitor::typeXtc, EpicsArchMonitor::iXtcVersion);
  char *pPoolBufferOverflowWarning =
    (char *) &dg + (int) (0.9 * EpicsArchMonitor::iMaxXtcSize);

  ////!!!debug
  //char sTimeText[64];
  //getLocalTime(tsCurrent, sTimeText);
  //printf("\nEpicsArchMonitor::writeToXtc() %s.%09u\n", sTimeText, (unsigned) tsCurrent.tv_nsec);

  bool bAnyPvWriteOkay      = false;
  bool bSomePvWriteError    = false;
  bool bSomePvNotConnected  = false; // PV not connected: Less serious than the "Write Error"
  for (int iPvName = 0; iPvName < iNumPv; iPvName++)
  {
    EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];

    if (! (bCtrlValue || epicsPvCur.isWriteEvent() ) )
      continue;

    if (_iDebugLevel >= 1)
      epicsPvCur.printPv();

    ////!!!debug
    //printf("Writing PV [%d] %s (%s) C %d\n", epicsPvCur.getPvId(), epicsPvCur.getPvDescription().c_str(),
    //  epicsPvCur.getPvName().c_str(), (int) (bCtrlValue) );

    XtcEpicsPv *pXtcEpicsPvCur = new(&dg.xtc) XtcEpicsPv(typeIdXtc, _src);

    int iFail = pXtcEpicsPvCur->setValue(epicsPvCur, bCtrlValue);

    if (iFail == 0)
      bAnyPvWriteOkay = true;
    else if (iFail == 2)  // Error code 2 means this PV has no connection
    {
      printf("%s (%s) failed to connect\n",
             epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());

      if (bCtrlValue)
      {     // If this happens in the "config" action (ctrl value is about to be written)
        epicsPvCur.release(); // release this PV and never update it again

        if ((*msg) == 0)
        {
          /*
           * Push the full list of problematic PVs, if possible.
           */
          if (_occPool.numberOfFreeObjects()) {
            (*msg) = new(&_occPool) UserMessage;
            (*msg)->append("EpicsArch: Some PVs not connected\n");
            (*msg)->append(epicsPvCur.getPvDescription().c_str());
            (*msg)->append(" (");
            (*msg)->append(epicsPvCur.getPvName().c_str());
            (*msg)->append(")\n");
          }
          else {
            printf("EpicsArchMonitor::writeToXtc occPool empty\n");
          }
        }
  else {
    int len = strlen(epicsPvCur.getPvDescription().c_str())+
      strlen(epicsPvCur.getPvName().c_str()+4);
    if ((*msg)->remaining() > len) {
      (*msg)->append(epicsPvCur.getPvDescription().c_str());
      (*msg)->append(" (");
      (*msg)->append(epicsPvCur.getPvName().c_str());
      (*msg)->append(")\n");
    }
  }
      }

      bSomePvNotConnected = true;
    }
    else
      bSomePvWriteError = true;

    char *pCurBufferPointer = (char *) dg.xtc.next();
    if (pCurBufferPointer > pPoolBufferOverflowWarning)
    {
      printf
        ("EpicsArchMonitor::writeToXtc(): Pool buffer size is too small.\n");
      printf
        ("EpicsArchMonitor::writeToXtc(): %d Pvs are stored in 90%% of the pool buffer (size = %d bytes)\n",
         iPvName + 1, EpicsArchMonitor::iMaxXtcSize);
      if (bCtrlValue && (*msg) == 0)
      {
        if (_occPool.numberOfFreeObjects()) {
          (*msg) = new(&_occPool) UserMessage;
          (*msg)->append("EpicsArch:PV data size too large\n");
        }
        else
          printf("EpicsArchMonitor occPool empty\n");
      }
      return 2;
    }
  }

  /// ca_pend For debug print
  //printf("Size of Data: Xtc %d Datagram %d Payload %d Total %d\n",
  //    sizeof(Xtc), sizeof(Datagram), pDatagram->xtc.sizeofPayload(),
  //    sizeof(Datagram) + pDatagram->xtc.sizeofPayload() );

  // checking errors, from the most serious one to the less serious one
  if (!bAnyPvWriteOkay)
    return 3;     // No PV has been outputted
  if (bSomePvWriteError)
    return 4;     // Some PV values have been outputted, but some has write error
  if (bSomePvNotConnected)
    return 5;     // Some PV values have been outputted, but some has not been connected

  return 0;     // All PV values are outputted successfully
}

int EpicsArchMonitor::validate(int iNumEventNode)
{
  ca_poll();

  const int iNumPv = _lpvPvList.size();

  int nNotConnected = 0;
  for (int iPvName = 0; iPvName < iNumPv; iPvName++)
  {
    EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];

    if (!epicsPvCur.isConnected()) {
      epicsPvCur.reconnect();
      printf("%s (%s) not connected\n",
             epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());
      nNotConnected++;
    }

    epicsPvCur.resetUpdates(iNumEventNode);
  }

  return nNotConnected;
}

int EpicsArchMonitor::resetUpdates(int iNumEventNode)
{
  const int iNumPv = _lpvPvList.size();
  for (int iPvName = 0; iPvName < iNumPv; iPvName++)
  {
    EpicsMonitorPv & epicsPvCur = _lpvPvList[iPvName];

    if (epicsPvCur.isConnected())
      epicsPvCur.resetUpdates(iNumEventNode);
  }
  return 0;
}

/*
* private static member functions
*/

int EpicsArchMonitor::_setupPvList(const Pds::PvConfigFile::TPvList & vPvList,
           TEpicsMonitorPvList & lpvPvList)
{
  if (vPvList.empty())
    return 0;
  if ((int) vPvList.size() >= iMaxNumPv)
    printf(
      "EpicsArchMonitor::_setupPvList(): Number of PVs (%zd) has reached the system capacity (%d), "
      "some PVs in the list may have been skipped.\n", vPvList.size(),
      iMaxNumPv);


  lpvPvList.resize(vPvList.size());
  for (int iPvName = 0; iPvName < (int) vPvList.size(); iPvName++)
  {
    EpicsMonitorPv & epicsPvCur = lpvPvList[iPvName];

    int iFail = epicsPvCur.init(iPvName, vPvList[iPvName].sPvName,
      vPvList[iPvName].sPvDescription, vPvList[iPvName].fUpdateInterval,
      _iNumEventNode);

    if (iFail != 0)
    {
      printf("EpicsArchMonitor::setupPvList()::EpicsMonitorPv::init(%s (%s)) failed\n",
        epicsPvCur.getPvDescription().c_str(), epicsPvCur.getPvName().c_str());
      return 1;
    }
  }

  /* flush all CA monitor requests */
  ca_flush_io();
  ca_pend_event(0.5);   // empirically, 0.5s is enough for 1000 PVs to be updated
  return 0;
}

}       // namespace Pds
