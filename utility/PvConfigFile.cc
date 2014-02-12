#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include "pds/utility/PvConfigFile.hh"

using std::string;
using std::stringstream;

namespace Pds
{

PvConfigFile::PvConfigFile(const std::string & sFnConfig, float fDefaultInterval, int iMaxDepth,
                           int iMaxNumPv, bool verbose) :
  _sFnConfig(sFnConfig),
  _fDefaultInterval(fDefaultInterval),
  _iMaxDepth(iMaxDepth),
  _iMaxNumPv(iMaxNumPv),
  _verbose(verbose)
{
}

PvConfigFile::~PvConfigFile()
{
}

int PvConfigFile::read(TPvList & vPvList, std::string& sConfigFileWarning)
{
  return (_readConfigFile(_sFnConfig, vPvList, sConfigFileWarning, _iMaxDepth));
}

int PvConfigFile::_readConfigFile(const std::string & sFnConfig,
        TPvList & vPvList, std::string& sConfigFileWarning, int maxDepth)
{
  if (maxDepth <= 0) {
    printf("%s: exceeded maximum include depth (%d)\n", __FUNCTION__, _iMaxDepth);
    printf("skipping file %s\n", sFnConfig.c_str());
    return 1;
  }

  std::ifstream ifsConfig(sFnConfig.c_str());
  if (!ifsConfig) {
    printf("failed to open file %s\n", sFnConfig.c_str());
    return 1;     // Cannot open file
  }
  if (_verbose) {
    printf("processing file %s\n", sFnConfig.c_str());
  }
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
      Pds::PvConfigFile::TFileList vsFileLst;
      _splitFileList(sLine, vsFileLst, _iMaxNumPv);

      for (int iPvFile = 0; iPvFile < (int) vsFileLst.size(); iPvFile++)
      {
        string sFnRef = vsFileLst[iPvFile];
        if (sFnRef[0] != '/')
          sFnRef = sFnPath + sFnRef;
        int iFail = _readConfigFile(sFnRef, vPvList, sConfigFileWarning, maxDepth - 1);
        if (iFail != 0)
        {
          printf("%s: Invalid file reference \"%s\", in file \"%s\":line %d\n",
             __FUNCTION__, sFnRef.c_str(), sFnConfig.c_str(), iLineNumber);
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

int PvConfigFile::_addPv(const string & sPvLine, string & sPvDescription,
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

  if ((int) vPvList.size() >= _iMaxNumPv)
  {
    printf("%s: Pv number > maximal allowable value (%d)\n",
     __FUNCTION__, _iMaxNumPv);
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

  vPvList.push_back(PvConfigFile::PvInfo(sPvName, sPvDescriptionUpdate, fInterval));
  bPvAdd = true;

  return iError;
}

int PvConfigFile::_splitFileList(const std::string & sFileList,
             Pds::PvConfigFile::TFileList & vsFileList, int iMaxNumPv)
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

int PvConfigFile::_getPvDescription(const std::string & sLine, std::string & sPvDescription)
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

int PvConfigFile::_updatePvDescription(const std::string& sPvName, const std::string& sFnConfig, int iLineNumber, std::string& sPvDescription, std::string& sConfigFileWarning)
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
    if (_verbose) {
      printf("%s: %s", __FUNCTION__, strMessage2);
    }
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
      if (_verbose) {
        printf("%s: %s", __FUNCTION__, strMessage2);
      }
      if (sConfigFileWarning.empty())
        sConfigFileWarning = strMessage2;

      _setPv.insert(sPvDescription);
      return 0;
    }
  }

  printf("%s: Cannot generate proper PV name for %s (%s).",
    __FUNCTION__, sPvDescription.c_str(), sPvName.c_str());

  sprintf(strMessage2, "%s No proper title found.\n", strMessage);
  printf("%s: %s", __FUNCTION__, strMessage2);
  sConfigFileWarning = strMessage2;

  return 1;
}

}       // namespace Pds
