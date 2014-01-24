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

  TPvList vPvList;
  int iFail = _readConfigFile(sFnConfig, vPvList, sConfigFileWarning);
  if (iFail != 0)
      throw "EpicsArchMonitor::EpicsArchMonitor()::readConfigFile(" +
        _sFnConfig + ") failed\n";

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

/*
* private static member functions
*/

int EpicsArchMonitor::_readConfigFile(const std::string & sFnConfig,
        TPvList & vPvList, std::string& sConfigFileWarning)
{
  printf("processing file %s\n", sFnConfig.c_str());

  std::ifstream ifsConfig(sFnConfig.c_str());
  if (!ifsConfig)
    return 1;     // Cannot open file

  string sFnPath;
  size_t uOffsetPathEnd = sFnConfig.find_last_of('/');
  if (uOffsetPathEnd != string::npos)
    sFnPath.assign(sFnConfig, 0, uOffsetPathEnd + 1);

  _setPv.clear();
  int iLineNumber = 0;

  string sPvDescription;
  while (!ifsConfig.eof())
  {
    ++iLineNumber;
    string sLine;
    std::getline(ifsConfig, sLine);

    if (sLine[0] == '*' ||
      (sLine[0] == '#' && sLine.size() > 1 && sLine[1] == '*') ) // description line
    {
      _getPvDescription(sLine, sPvDescription);
      continue;
    }
    if (sLine[0] == '#')
    {
      sPvDescription.clear();
      continue;   // skip comment lines that begin with '#'
    }
    if (sLine[0] == '<')
    {
      sLine[0] = ' ';
      TFileList vsFileLst;
      _splitFileList(sLine, vsFileLst);

      for (int iPvFile = 0; iPvFile < (int) vsFileLst.size(); iPvFile++)
      {
        string sFnRef = vsFileLst[iPvFile];
        if (sFnRef[0] != '/')
          sFnRef = sFnPath + sFnRef;
        int iFail = _readConfigFile(sFnRef, vPvList, sConfigFileWarning);
        if (iFail != 0)
        {
          printf("EpicsArchMonitor::_readConfigFile(): Invalid file reference \"%s\", in file \"%s\":line %d\n",
             sFnRef.c_str(), sFnConfig.c_str(), iLineNumber);
        }
      }
      continue;
    }

    bool bAddPv;
    int iError = _addPv(sLine, sPvDescription, vPvList, bAddPv, sFnConfig, iLineNumber, sConfigFileWarning);

    if (iError != 0)
      return 1;

    if (!bAddPv)
      sPvDescription.clear();
  }

  return 0;
}

int EpicsArchMonitor::_setupPvList(const TPvList & vPvList,
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

int EpicsArchMonitor::_addPv(const string & sPvLine, string & sPvDescription,
           TPvList & vPvList, bool & bPvAdd,
           const std::string& sFnConfig, int iLineNumber, std::string& sConfigFileWarning)
{
  bPvAdd = false;

  const char sPvLineSeparators[] = " ,;\t\r\n";
  size_t uOffsetPv = sPvLine.find_first_not_of(sPvLineSeparators, 0);
  if (uOffsetPv == string::npos)
    return 0;

  if (sPvLine[uOffsetPv] == '#')
    return 0;

  if ((int) vPvList.size() >= iMaxNumPv)
  {
    printf("EpicsArchMonitor::_addPv(): Pv number > maimal allowable value (%d)\n",
     iMaxNumPv);
    return 1;
  }

  string  sPvName;
  float   fInterval = _fDefaultInterval;

  size_t uOffsetSeparator =
    sPvLine.find_first_of(sPvLineSeparators, uOffsetPv + 1);
  if (uOffsetSeparator == string::npos)
  {
    sPvName   = sPvLine.substr(uOffsetPv, string::npos);
  }
  else
  {
    sPvName   = sPvLine.substr(uOffsetPv, uOffsetSeparator - uOffsetPv);
    size_t uOffsetInterval =
      sPvLine.find_first_not_of(sPvLineSeparators, uOffsetSeparator + 1);
    if (uOffsetInterval != string::npos)
      fInterval = strtof(sPvLine.c_str() + uOffsetInterval, NULL);
  }

  string sPvDescriptionUpdate = sPvDescription;
  int iError = _updatePvDescription(sPvName, sFnConfig, iLineNumber, sPvDescriptionUpdate, sConfigFileWarning);

  vPvList.push_back(PvInfo(sPvName, sPvDescriptionUpdate, fInterval));
  bPvAdd = true;

  return iError;
}

int EpicsArchMonitor::_splitFileList(const std::string & sFileList,
             TFileList & vsFileList)
{
  static const char sFileListSeparators[] = " ,;\t\r\n";
  size_t uOffsetStart = sFileList.find_first_not_of(sFileListSeparators, 0);
  while (uOffsetStart != string::npos)
  {
    if (sFileList[uOffsetStart] == '#')
      break;      // skip the remaining characters

    size_t uOffsetEnd =
      sFileList.find_first_of(sFileListSeparators, uOffsetStart + 1);

    if (uOffsetEnd == string::npos)
    {
      if ((int) vsFileList.size() < iMaxNumPv)
        vsFileList.push_back(sFileList.substr(uOffsetStart, string::npos));

      break;
    }

    if ((int) vsFileList.size() < iMaxNumPv)
      vsFileList.push_back(sFileList.substr(uOffsetStart, uOffsetEnd - uOffsetStart));
    else
      break;

    uOffsetStart = sFileList.find_first_not_of(sFileListSeparators, uOffsetEnd + 1);
  }
  return 0;
}

int EpicsArchMonitor::_getPvDescription(const std::string & sLine, std::string & sPvDescription)
{
  const char sPvDecriptionSeparators[] = " \t*#";
  size_t uOffsetStart = sLine.find_first_not_of(sPvDecriptionSeparators, 0);
  if (uOffsetStart == string::npos)
  {
    sPvDescription.clear();
    return 0;
  }

  size_t uOffsetEnd = sLine.find("#", uOffsetStart+1);
  size_t uOffsetTrail = sLine.find_last_not_of(" \t#", uOffsetEnd );
  if (uOffsetTrail == string::npos)
    sPvDescription.clear();
  else
    sPvDescription = sLine.substr(uOffsetStart, uOffsetTrail - uOffsetStart + 1);

  return 0;
}

int EpicsArchMonitor::_updatePvDescription(const std::string& sPvName, const std::string& sFnConfig, int iLineNumber, std::string& sPvDescription, std::string& sConfigFileWarning)
{
  char strMessage [256];
  char strMessage2[256];
  if (!sPvDescription.empty())
  {
    if ( _setPv.find(sPvDescription) == _setPv.end() )
    {
      _setPv.insert(sPvDescription);
      return 0;
    }
    sprintf( strMessage, "%s has duplicated title \"%s\".", sPvName.c_str(), sPvDescription.c_str());
    sPvDescription  += '-' + sPvName;
  }
  else
  {
    sPvDescription = sPvName;
    sprintf( strMessage, "%s has no title.", sPvName.c_str());
  }

  if ( _setPv.find(sPvDescription) == _setPv.end() )
  {
    sprintf(strMessage2, "%s\nUse \"%s\"\n", strMessage, sPvDescription.c_str() );
    printf("EpicsArchMonitor::_updatePvDescription(): %s", strMessage2);
    if (sConfigFileWarning.empty())
      sConfigFileWarning = strMessage2;

    _setPv.insert(sPvDescription);
    return 0;
  }

  static const int iMaxPvSerial = 10000;
  for (int iPvSerial = 2; iPvSerial < iMaxPvSerial; ++iPvSerial)
  {
    stringstream sNumber;
    sNumber << iPvSerial;

    string sPvDesecriptionNew;
    sPvDesecriptionNew = sPvDescription + '-' + sNumber.str();

    if ( _setPv.find(sPvDesecriptionNew) == _setPv.end() )
    {
      sPvDescription = sPvDesecriptionNew;

      sprintf(strMessage2, " %s Use %s\n", strMessage, sPvDescription.c_str() );
      printf("EpicsArchMonitor::_updatePvDescription(): %s", strMessage2);
      if (sConfigFileWarning.empty())
        sConfigFileWarning = strMessage2;

      _setPv.insert(sPvDescription);
      return 0;
    }
  }

  printf("EpicsArchMonitor::_updatePvDescription(): Cannot generate proper PV name for %s (%s).",
    sPvDescription.c_str(), sPvName.c_str());

  sprintf(strMessage2, "%s No proper title found.\n", strMessage);
  printf("EpicsArchMonitor::_updatePvDescription(): %s", strMessage2);
  sConfigFileWarning = strMessage2;

  return 1;
}

}       // namespace Pds
