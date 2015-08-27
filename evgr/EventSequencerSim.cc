#include "EventSequencerSim.hh"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>

#include "pdsdata/xtc/TimeStamp.hh"

using namespace std;

static void* thread_accept(void* arg) {
  EventSequencerSim* pEvtSeq = (EventSequencerSim*) arg;
  int iError = pEvtSeq->accept();
  pthread_exit( (void*) iError );
}

struct ArgProcess {
  EventSequencerSim*  pEvtSeq;
  int                 fClient;
};

static void* thread_process(void* arg) {
  ArgProcess* pArg = (ArgProcess*) arg;
  pArg->pEvtSeq->process(pArg->fClient);
  return 0;
}

EventSequencerSim::EventSequencerSim() : _bStopThreads(false), _threadAccept(-1), _numThreadProc(0),
  _threadSeqUpdate(-1), _curFiducial(0) {
  if (initPvMap())
    throw runtime_error("Pv map init Failed");


  _cmdBuffer = new char [BUFFER_SIZE];
  _outBuffer = new char [BUFFER_SIZE];

  for (int iSeq=0; iSeq< numSequencer; ++iSeq) {
    _lSeq[iSeq].iSeqId = iSeq+1;
  }

  pthread_mutex_init ( &_mutexUpdate, NULL);
  pthread_rwlock_init( &_rwlockPv,  NULL);

  if (startPvServer())
    throw runtime_error("Start PV Server Failed");

  _lSeqEvents.reserve(256);

  if (startSeqUpdater())
    throw runtime_error("Start Seq Updater Failed");
}

EventSequencerSim::~EventSequencerSim() {
  if (_threadAccept != -1U)
  {
    void* pError;
    pthread_join(_threadAccept, &pError);
  }

  pthread_rwlock_destroy(&_rwlockPv);
  pthread_mutex_destroy (&_mutexUpdate);

  delete _outBuffer;
  delete _cmdBuffer;
}

int EventSequencerSim::startPvServer() {
  if (
    (_fSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return 1;

  int optval=1;
  if (setsockopt(_fSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    printf("EventSequencerSim::startPvServer(): setsockopt(SO_REUSEADDR,1) failed : %s\n", strerror(errno));
    close(_fSocket);
  }

  sockaddr_in sockaddrSrc;
  sockaddrSrc.sin_family      = AF_INET;
  sockaddrSrc.sin_addr.s_addr = INADDR_ANY;
  sockaddrSrc.sin_port        = htons(iSocketPort);
  if (
    bind(_fSocket, (struct sockaddr*) &sockaddrSrc, sizeof(sockaddrSrc)) < 0) {
    printf("EventSequencerSim::startPvServer(): bind(port %d) failed: %s\n", iSocketPort, strerror(errno));
    close(_fSocket);
    return 2;
  }

  if (
    listen(_fSocket, SOMAXCONN) < 0) {
    printf("EventSequencerSim::startPvServer(): listen() failed: %s\n", strerror(errno));
    close(_fSocket);
    return 3;
  }

  int iError = pthread_create(&_threadAccept, NULL, ::thread_accept, this);
  if (iError != 0) {
    printf("EventSequencerSim::startPvServer(): pthread_create() failed: %d %s\n", iError, strerror(errno));
    close(_fSocket);
    return 4;
  }

  return 0;
}

//static string addressToStr( unsigned int uAddr );
static void split_string(const string& s, const char delim[], vector<string>& ls);

int EventSequencerSim::accept() {
  printf("Accepting new connection in port %d f %d\n", iSocketPort, _fSocket);

  timeval timeSleepMicroOrg = {0, 100000 }; // 100 milliseconds
  fd_set fdsOrg;
  FD_ZERO(&fdsOrg);
  FD_SET (_fSocket, &fdsOrg);

  while (!_bStopThreads) {
    timeval timeSleepMicro = timeSleepMicroOrg;
    fd_set fdsRead         = fdsOrg;
    if (select( 1 + _fSocket, &fdsRead, NULL, NULL, &timeSleepMicro) > 0) {
      struct sockaddr_in sockAddr;
      socklen_t          len = sizeof(sockAddr);
      int fClient = ::accept(_fSocket, (struct sockaddr*) &sockAddr, &len);
      if (fClient == -1) {
        printf("EventSequencerSim::thread_accept():: accept() failed: %s\n", strerror(errno));
        close(_fSocket);
        return 1;
      }

      //unsigned int uSockAddr = ntohl(sockAddr.sin_addr.s_addr);
      //unsigned int uSockPort = (unsigned int )ntohs(sockAddr.sin_port);
      //!!!debug
      //printf( "{%d} Client [%d] %s:%u ", _numThreadProc, fClient, addressToStr(uSockAddr).c_str(), uSockPort );

      pthread_t threadProcess;
      ArgProcess argProcess = {this, fClient};
      pthread_create(&threadProcess, NULL, ::thread_process, &argProcess);
      pthread_detach(threadProcess);
    }
  }

  //printf("!!! STOP accepting new connection in port %d\n", iSocketPort); //!!!debug

  return 0;
}

int EventSequencerSim::process(int fClient) {
  pthread_mutex_lock(&_mutexUpdate);
  ++_numThreadProc;
  pthread_mutex_unlock(&_mutexUpdate);

  char* pBuf        = _cmdBuffer;
  int   remainSize  = BUFFER_SIZE-1;
  int   len         = 0;
  while (remainSize > 0) {
    int len1 = recv(fClient, pBuf, remainSize, 0);
    if (len1 == -1)  {
      printf("EventSequencerSim::process(): recv() error: %s\n", strerror(errno));
      close(fClient);
      return 1;
    } else if (len1 == remainSize) {
      printf("EventSequencerSim::process(): buffer size %d not enough for recv()\n", BUFFER_SIZE);
      close(fClient);
      return 2;
    }
    len += len1;
    if (_cmdBuffer[len-1] == '\n')
      break;
    pBuf        += len1;
    remainSize  -= len1;
  }

  _cmdBuffer[len] = 0;
  string sInput = _cmdBuffer;

  ////!!!debug
  //printf(" <L %d> %s\n", (int) sInput.size(), sInput.c_str());

  static const char delimiters[] = " \t\n\r";
  vector<string> lsParam;
  split_string( sInput, delimiters, lsParam );

  do {
    if (lsParam.size() == 0)
      break;

    string& sCmd = lsParam[0];
    if (sCmd == "LIST"){
      vector<string> lsPv;
      listPv("", lsPv);
      send(fClient, "OKAY\n", 5, MSG_NOSIGNAL);

      stringstream sstream;
      sstream << lsPv.size();
      string sLen = sstream.str();
      sLen += "\n";

      send(fClient, sLen.c_str(), sLen.length(), MSG_NOSIGNAL);
      for (size_t i=0; i<lsPv.size(); ++i) {
        lsPv[i] += "\n";
        send(fClient, lsPv[i].c_str(), lsPv[i].length(), MSG_NOSIGNAL);
      }
      send(fClient, "-END-\n", 6, MSG_NOSIGNAL);
    }
    else if (sCmd == "GET"){
      if (lsParam.size() < 2){
        printf("*Illgal GET syntax\n");
        send(fClient, "*SYNTAX ERROR\n", 14, MSG_NOSIGNAL);
        send(fClient, "-END-\n", 6, MSG_NOSIGNAL);
        break;
      }

      string sVal;
      int iFail = getPv(lsParam[1], sVal);

      if (iFail != 0) {
        printf("GET failed: PV %s does not exist.\n", lsParam[1].c_str());
        send(fClient, "*PV NOT FOUND\n", 14, MSG_NOSIGNAL);
      }
      else {
        ////!!!debug
        //printf("GET %s\n", sVal.c_str());
        send(fClient, "OKAY\n", 5, MSG_NOSIGNAL);
        send(fClient, sVal.c_str(), sVal.length(), MSG_NOSIGNAL);
      }
      send(fClient, "-END-\n", 6, MSG_NOSIGNAL);
    }
    else if (sCmd == "SET"){
      if (lsParam.size() < 3){
        printf("Illgal SET syntax\n");
        send(fClient, "*SYNTAX ERROR\n", 14, MSG_NOSIGNAL);
        send(fClient, "-END-\n", 6, MSG_NOSIGNAL);
        break;
      }
      //printf("Running command SET PV %s to value %s\n", lsParam[1].c_str(), lsParam[2].c_str());

      string sVal;
      int iFail = setPv(lsParam[1], lsParam, 2, sVal);

      if (iFail != 0) {
        printf("SET failed: PV %s does not exist.\n", lsParam[1].c_str());
        send(fClient, "*PV NOT FOUND\n", 14, MSG_NOSIGNAL);
      }
      else {
        ////!!!debug
        //printf("SET %s\n", sVal.c_str());
        send(fClient, "OKAY\n", 5, MSG_NOSIGNAL);
        send(fClient, sVal.c_str(), sVal.length(), MSG_NOSIGNAL);
      }
      send(fClient, "-END-\n", 6, MSG_NOSIGNAL);
    } else { // unrecognized command
      printf("Illegal command: %s\n", sCmd.c_str());
      send(fClient, "*ILLEGAL COMMAND ", 17, MSG_NOSIGNAL);
      send(fClient, sCmd.c_str(), sCmd.length(), MSG_NOSIGNAL);
      send(fClient, "\n-END-\n", 7, MSG_NOSIGNAL);
    }

  } while(false);

  //if ((unsigned)len > sizeof(_cmdBuffer)-9)
  //  len = sizeof(_cmdBuffer)-9;
  //memmove(_cmdBuffer+8, _cmdBuffer, len);
  //memcpy (_cmdBuffer, "Result: ", 8);
  //len += 8;
  //len = send(fClient, _cmdBuffer, len, 0);
  //if (len == -1)
  //{
  //  printf("EventSequencerSim::process(): send() error: %s\n", strerror(errno));
  //  close(fClient);
  //}

  close(fClient);

  pthread_mutex_lock(&_mutexUpdate);
  --_numThreadProc;
  pthread_mutex_unlock(&_mutexUpdate);
  return 0;
}

int EventSequencerSim::initPvMap() {
  typedef TPvMap::value_type PV;

  _mapPv.insert(PV("EVNT:SYS0:1:LCLSBEAMRATE", "120.0"));
  _mapPv.insert(PV("PATT:SYS0:1:TESTBURST.N", 0));   // Set to 0 to elminate dependency on beam
  _mapPv.insert(PV("PATT:SYS0:1:TESTBURSTRATE", 0)); // 0:Full 1:30 2:10 3:5 4:1 5:0.5 6:pattern 7:off

  _mapPv.insert(PV("PATT:SYS0:1:MPSBURSTCTRL", "Off"));
  _mapPv.insert(PV("PATT:SYS0:1:MPSBURSTCNTMAX", 0));
  _mapPv.insert(PV("PATT:SYS0:1:MPSBURSTRATE", 0)); // 0:Full 1:30 2:10 3:5 4:1 5:0.5 6:pattern 7:off
  _mapPv.insert(PV("PATT:SYS0:1:MPSBURSTCNT", 0));

  _mapPv.insert(PV("IOC:BSY0:MP01:REQBYKIKBRST", "No"));
  _mapPv.insert(PV("IOC:BSY0:MP01:REQPCBRST", "No"));
  _mapPv.insert(PV("IOC:IN20:EV01:BYKIK_ABTACT", 0));
  _mapPv.insert(PV("IOC:IN20:EV01:BYKIK_ABTPRD", 0));

  string sMccIocPre = "ECS:SYS0:";
  _mapPv.insert(PV(sMccIocPre+"0:BEAM_OWNER_ID", 6));
  _mapPv.insert(PV(sMccIocPre+"1:HTUCH_NAME", "AMO"));
  _mapPv.insert(PV(sMccIocPre+"2:HTUCH_NAME", "SXR"));
  _mapPv.insert(PV(sMccIocPre+"3:HTUCH_NAME", "XPP"));
  _mapPv.insert(PV(sMccIocPre+"4:HTUCH_NAME", "XCS"));
  _mapPv.insert(PV(sMccIocPre+"5:HTUCH_NAME", "CXI"));
  _mapPv.insert(PV(sMccIocPre+"6:HTUCH_NAME", "MEC"));
  _mapPv.insert(PV(sMccIocPre+"7:HTUCH_NAME", "SP7"));
  _mapPv.insert(PV(sMccIocPre+"8:HTUCH_NAME", "SP8"));

  for (int i=1; i<=8; ++i){
    string sMccHutchPre = sMccIocPre + (char) ('0'+i) + ':';
    _mapPv.insert(PV(sMccHutchPre+"HUTCH_ID", i));
    _mapPv.insert(PV(sMccHutchPre+"SEQ.A", vector<int>(1))); // Event Codes
    _mapPv.insert(PV(sMccHutchPre+"SEQ.B", vector<int>(1))); // Beam Delay
    _mapPv.insert(PV(sMccHutchPre+"SEQ.C", vector<int>(1))); // Fiducial Delay
    _mapPv.insert(PV(sMccHutchPre+"SEQ.D", vector<int>(1))); // Burst Count. -1: forever, -2: stop
    _mapPv.insert(PV(sMccHutchPre+"SEQ.PROC", 0));
    _mapPv.insert(PV(sMccHutchPre+"LEN", 0));
    _mapPv.insert(PV(sMccHutchPre+"PLYCTL", 0)); // 0: Stop,    1: Play
    _mapPv.insert(PV(sMccHutchPre+"PLSTAT", "Stopped")); // 0: Stopped, 1: Waiting,  2: Playing
    _mapPv.insert(PV(sMccHutchPre+"PLYMOD", 0)); // 0: Once,    1: Repeat N, 2: Repeat Forever
    _mapPv.insert(PV(sMccHutchPre+"REPCNT", 0));

    /*
       Error Type:
         0: Internal Error  1: Unknown Error  2: Valid Sequence
         3: Inv Event Code  4: Inv Beam Delay 5: Inv Fid Delay
         6: Zero Length     7: Inv Hutch ID   8: Inv EC Hutch ID
     */
    _mapPv.insert(PV(sMccHutchPre+"ERRTYP", "Zero Length"));

    _mapPv.insert(PV(sMccHutchPre+"ERRIDX", 0));
    _mapPv.insert(PV(sMccHutchPre+"PLYCNT", 0));
    _mapPv.insert(PV(sMccHutchPre+"TPLCNT", 0));
    _mapPv.insert(PV(sMccHutchPre+"CURSTP", 0));

    /*
        Sync Marker 0:0.5, 1:1, 2:5, 3:10, 4:30, 5:60, 6:120, 7:360
     */
    _mapPv.insert(PV(sMccHutchPre+"SYNCMARKER", 3)); // default to sync at 10Hz

    // 0: Immediate 1: Next Tick
    _mapPv.insert(PV(sMccHutchPre+"SYNCNEXTTICK", 0));

    // Beam Pulse Reques  0:Timeslot, 1:Beam
    _mapPv.insert(PV(sMccHutchPre+"BEAMPULSEREQ", 0));
  }

  return 0;
}

int EventSequencerSim::setPv(const std::string& sPv, const std::vector<std::string>& lsParam, int iIndexStart,
                              std::string& sVal) {
  pthread_rwlock_wrlock( &_rwlockPv );

  TPvValue::TPvType   etype = TPvValue::TYPE_INT;
  TPvMap::iterator    it    = _mapPv.find(sPv);
  if (it != _mapPv.end()) {
    etype = it->second.eType;
    //printf("SET PV %s : DEL Old Type %d Val %s\n", it->first.c_str(), (int) it->second.eType, it->second.to_string().c_str());
    _mapPv.erase(it);
  }

  //printf("SET PV %s : Set New Val [%s]\n", sPv.c_str(), lsParam[iIndexStart].c_str());

  typedef TPvMap::value_type PV;
  if ((int) lsParam.size() == iIndexStart+1 && etype != TPvValue::TYPE_INTARRAY){
    string::size_type pos = lsParam[iIndexStart].find_first_not_of("1234567890-\r\n");
    ////!!!debug
    //printf("check param non-digit pos %d <0x%x>\n", pos, (int)lsParam[iIndexStart][pos]);

    if ( pos == string::npos )
      _mapPv.insert(PV(sPv, strtol(lsParam[iIndexStart].c_str(), NULL, 0)) );
    else
      _mapPv.insert(PV(sPv, lsParam[iIndexStart]) );
  } else{
    vector<int> vecInt;
    for (unsigned i=iIndexStart; i<lsParam.size(); ++i){
      vecInt.push_back( strtol(lsParam[i].c_str(), NULL, 0) );
    }
    _mapPv.insert(PV(sPv, vecInt));
  } // else // if (lsParam.size() == iIndexStart+1){

  it = _mapPv.find(sPv);
  if (it == _mapPv.end())
  {
    pthread_rwlock_unlock( &_rwlockPv );
    return 1;
  }

  sprintf(_outBuffer, "PV %s Type %d Val %s\n", it->first.c_str(), (int) it->second.eType, it->second.to_string().c_str());
  sVal = _outBuffer;

  pthread_rwlock_unlock( &_rwlockPv );

  notifySequencer(sPv);
  return 0;
}

int EventSequencerSim::getPv(const std::string& sPv, std::string& sVal) {
  pthread_rwlock_rdlock( &_rwlockPv );

  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    pthread_rwlock_unlock( &_rwlockPv );
    return 1;
  }

  sprintf(_outBuffer, "PV %s Type %d Val %s\n", it->first.c_str(), (int) it->second.eType, it->second.to_string().c_str());
  sVal = _outBuffer;

  pthread_rwlock_unlock( &_rwlockPv );
  return 0;
}

int EventSequencerSim::listPv (const std::string& sPvMatch, vector<string>& lsPv) {
  pthread_rwlock_rdlock( &_rwlockPv );

  for (TPvMap::iterator it = _mapPv.begin(); it != _mapPv.end(); ++it){
    printf("PV %s Type %d Val %s\n", it->first.c_str(), (int) it->second.eType, it->second.to_string().c_str());
    lsPv.push_back(it->first);
  }
  printf("# %d PVs printed.\n", (int) _mapPv.size());

  pthread_rwlock_unlock( &_rwlockPv );
  return 0;
}

int EventSequencerSim::notifySequencer(const std::string& sPv) {
  if (sPv == "EVNT:SYS0:1:LCLSBEAMRATE") {
    for (int iSeq=0; iSeq < numSequencer; ++iSeq)
      _lSeq[iSeq].bPvUpdated = true;
    return 0;
  }

  if (sPv.find("ECS:SYS0:") != 0)
    return 0;

  int iSeqId = sPv[9] - '1';
  if (iSeqId < 0 || iSeqId > numSequencer)
    return 0;

  string sSeqField = sPv.substr(11);
  if (sSeqField == "SYNCMARKER"   ||
      sSeqField == "SYNCNEXTTICK" ||
      sSeqField == "BEAMPULSEREQ") {
    _lSeq[iSeqId].bPvUpdated = true;
  }
  else if (sSeqField == "PLYMOD" ||
           sSeqField == "LEN"    ||
           sSeqField == "REPCNT")
  {
    _lSeq[iSeqId].play(0, _curFiducial);
    _lSeq[iSeqId].bPvUpdated = true;
  }
  else if (sSeqField == "PLYCTL") {
    int iPlayCtl;
    pthread_rwlock_rdlock( &_rwlockPv );
    getPvAsync(sPv, iPlayCtl);
    pthread_rwlock_unlock( &_rwlockPv );
    _lSeq[iSeqId].play( (iPlayCtl? 1: 0), _curFiducial);
    _lSeq[iSeqId].bPvUpdated = true;
  }
  else if (sSeqField == "SEQ.PROC") {
    _lSeq[iSeqId].play(0, _curFiducial);

    string sHutchPrev = sPv.substr(0,11);
    int iLen;
    vector<int>* pVecA;
    vector<int>* pVecB;
    vector<int>* pVecC;
    vector<int>* pVecD;
    pthread_rwlock_rdlock( &_rwlockPv );
    getPvAsync(sHutchPrev + "LEN", iLen);
    getPvAsync(sHutchPrev + "SEQ.A", pVecA);
    getPvAsync(sHutchPrev + "SEQ.B", pVecB);
    getPvAsync(sHutchPrev + "SEQ.C", pVecC);
    getPvAsync(sHutchPrev + "SEQ.D", pVecD);
    pthread_rwlock_unlock( &_rwlockPv );
    _lSeq[iSeqId].loadSeq(iLen, *pVecA, *pVecB, *pVecC, *pVecD);
  }
  return 0;
}

int EventSequencerSim::setPvAsync(const std::string& sPv, int  iVal) {
  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    printf("EventSequencerSim::setPv(%s): Cannot find PV\n", sPv.c_str());
    return 1;
  }

  TPvValue& val = it->second;
  if (val.eType != TPvValue::TYPE_INT) {
    printf("EventSequencerSim::setPv(%s): Wrong type, not INT\n", sPv.c_str());
    return 2;
  }

  val.iVal = iVal;
  return 0;
}

int EventSequencerSim::setPvAsync(const std::string& sPv, const std::string& sVal) {
  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    printf("EventSequencerSim::setPv(%s): Cannot find PV\n", sPv.c_str());
    return 1;
  }

  TPvValue& val = it->second;
  if (val.eType != TPvValue::TYPE_STRING) {
    printf("EventSequencerSim::setPv(%s): Wrong type, not STRING\n", sPv.c_str());
    return 2;
  }

  *val.pStr = sVal;
  return 0;
}

int EventSequencerSim::getPvAsync(const std::string& sPv, int& iVal) {
  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    printf("EventSequencerSim::getPv(%s, INT): Cannot find PV\n", sPv.c_str());
    return 1;
  }

  TPvValue& val = it->second;
  if (val.eType != TPvValue::TYPE_INT) {
    printf("EventSequencerSim::getPv(%s, INT): Wrong type, not INT\n", sPv.c_str());
    return 2;
  }

  iVal = val.iVal;
  return 0;
}

int EventSequencerSim::getPvAsync(const std::string& sPv, std::string*& pStr) {
  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    printf("EventSequencerSim::getPv(%s, STRING): Cannot find PV\n", sPv.c_str());
    return 1;
  }

  TPvValue& val = it->second;
  if (val.eType != TPvValue::TYPE_STRING) {
    printf("EventSequencerSim::getPv(%s, STRING): Wrong type, not INT\n", sPv.c_str());
    return 2;
  }

  pStr = val.pStr;
  return 0;
}

int EventSequencerSim::getPvAsync(const std::string& sPv, std::vector<int>*& pliVal) {
  TPvMap::iterator it = _mapPv.find(sPv);
  if (it == _mapPv.end()) {
    printf("EventSequencerSim::getPv(%s, INTARRAY): Cannot find PV\n", sPv.c_str());
    return 1;
  }

  TPvValue& val = it->second;
  if (val.eType != TPvValue::TYPE_INTARRAY) {
    printf("EventSequencerSim::getPv(%s, INTARRAY): Wrong type, not INTARRAY\n", sPv.c_str());
    return 2;
  }

  pliVal = val.pVecInt;
  return 0;
}

static void* thread_seqUpdate(void* arg) {
  EventSequencerSim* pEvtSeq = (EventSequencerSim*) arg;
  int iError = pEvtSeq->seqUpdate();
  pthread_exit( (void*) iError );
}

int EventSequencerSim::startSeqUpdater() {
  int iError = pthread_create(&_threadSeqUpdate, NULL, ::thread_seqUpdate, this);
  if (iError != 0) {
    printf("EventSequencerSim::startSeqUpdater(): pthread_create() failed: %d %s\n", iError, strerror(errno));
    close(_fSocket);
    return 1;
  }

  return 0;
}

int EventSequencerSim::seqUpdate() {
  while (!_bStopThreads) {
    for (int iSeq=0; iSeq < numSequencer; ++iSeq) {
      TEventSequencer& seq = _lSeq[iSeq];
      while (seq.bSeqStatusUpdated)
        updatePvFromSeq(seq);
      while (seq.bPvUpdated)
        updateSeqFromPv(seq);
    }

    timeval timeSleepMicro = {0, 5000}; // 5 milliseconds
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
  }
  return 0;
}


int EventSequencerSim::updatePvFromSeq(TEventSequencer& seq) {
  string sMccHutchPre = string("ECS:SYS0:") + (char) ('0'+seq.iSeqId) + ':';
  static const string lsPlayStatus[] = {"Stopped", "Waiting", "Playing"};
  static const string lsErrType   [] = {
    "Internal Error", "Unknown Error", "Valid Sequence",
    "Inv Event Code", "Inv Beam Delay", "Inv Fid Delay",
    "Zero Length", "Inv Hutch ID", "Inv EC Hutch ID"
  };


  pthread_rwlock_wrlock( &_rwlockPv );
  ////!!!debug
  //printf("  >>  updatePvFromSeq(seq %d)\n", seq.iSeqId);

  setPvAsync( sMccHutchPre + "CURSTP", seq.iCurStep );
  setPvAsync( sMccHutchPre + "PLSTAT", lsPlayStatus[seq.iPlayStatus] );
  setPvAsync( sMccHutchPre + "PLYCNT", seq.iPlayCount );
  setPvAsync( sMccHutchPre + "TPLCNT", seq.iTotalPlayCount );
  setPvAsync( sMccHutchPre + "ERRTYP", lsErrType[seq.iErrType] );
  setPvAsync( sMccHutchPre + "ERRIDX", seq.iErrIndex );
  pthread_rwlock_unlock( &_rwlockPv );

  seq.bSeqStatusUpdated = false;
  return 0;
}

int EventSequencerSim::updateSeqFromPv(TEventSequencer& seq) {
  string  sMccHutchPre = string("ECS:SYS0:") + (char) ('0'+seq.iSeqId) + ':';
  string* psBeamRate;
  pthread_rwlock_rdlock( &_rwlockPv );
  ////!!!debug
  //printf("    << updateSeqFromPv(seq %d)", seq.iSeqId);

  getPvAsync( "EVNT:SYS0:1:LCLSBEAMRATE", psBeamRate );
  seq.fBeamRate = strtof(psBeamRate->c_str(), NULL);

  getPvAsync( sMccHutchPre + "PLYMOD", seq.iPlayMode );
  getPvAsync( sMccHutchPre + "REPCNT", seq.iRepeatCount );
  getPvAsync( sMccHutchPre + "SYNCMARKER"  , seq.iSyncMarker );
  getPvAsync( sMccHutchPre + "SYNCNEXTTICK", seq.iNextSync );
  getPvAsync( sMccHutchPre + "BEAMPULSEREQ", seq.iBeamPulseReq );
  pthread_rwlock_unlock( &_rwlockPv );

  seq.bPvUpdated = false;
  return 0;
}

int EventSequencerSim::getCodeDelay(int iEvent) {
  return 11850 + iEvent;
}

int EventSequencerSim::getSeqEvent(unsigned fiducial, int& nSeqEvents, int*& lSeqEvents) {
  _curFiducial = fiducial;

  _lSeqEvents.clear();
  for (int iSeq=0; iSeq < numSequencer; ++iSeq) {
    TEventSequencer& seq = _lSeq[iSeq];
    seq.getSeqEvent(fiducial, _lSeqEvents);
  }

  sort(_lSeqEvents.begin(), _lSeqEvents.end());

  nSeqEvents = _lSeqEvents.size();
  lSeqEvents = &_lSeqEvents[0];

  ////!!!debug
  //if (nSeqEvents > 0)
  //  printf("EventSequencerSim::getSeqEvent() : got %d events , first %d\n",  nSeqEvents,  lSeqEvents[0]);
  return 0;
}

static void split_string(const string& s, const char delim[], vector<string>& ls) {
  size_t pos = s.find_first_not_of(delim);
  while (pos != string::npos) {
    size_t posEnd = s.find_first_of(delim, pos);
    ls.push_back( s.substr( pos, posEnd == string::npos ? string::npos : posEnd-pos ) );
    pos = s.find_first_not_of(delim, posEnd);
  }
}

//static string addressToStr( unsigned int uAddr ){
//  unsigned int uNetworkAddr = htonl(uAddr);
//  const unsigned char* pcAddr = (const unsigned char*) &uNetworkAddr;
//  stringstream sstream;
//  sstream <<
//          (int) pcAddr[0] << "." <<
//          (int) pcAddr[1] << "." <<
//          (int) pcAddr[2] << "." <<
//          (int) pcAddr[3];

//  return sstream.str();
//}

EventSequencerSim::TPvValue::TPvValue(int iVal)            : eType(TYPE_INT)     , iVal(iVal)      {}
EventSequencerSim::TPvValue::TPvValue(const char*   sVal)  : eType(TYPE_STRING)  , pStr(new std::string(sVal))      {}
EventSequencerSim::TPvValue::TPvValue(const string& sVal)  : eType(TYPE_STRING)  , pStr(new std::string(sVal))      {}
EventSequencerSim::TPvValue::TPvValue(int iCount, int iVal): eType(TYPE_INTARRAY), pVecInt(new std::vector<int>(iCount,iVal)) {}
EventSequencerSim::TPvValue::TPvValue(const vector<int>& vVal)  : eType(TYPE_INTARRAY), pVecInt(new std::vector<int>(vVal)) {}
EventSequencerSim::TPvValue::TPvValue(const TPvValue& ref) : eType(ref.eType) {
  switch(eType)
  {
    case TYPE_INT:
      iVal = ref.iVal;
      break;
    case TYPE_STRING:
      pStr = new string(*ref.pStr);
      break;
    case TYPE_INTARRAY:
      pVecInt = new vector<int>(*ref.pVecInt);
      break;
  };
}

EventSequencerSim::TPvValue& EventSequencerSim::TPvValue::swap(TPvValue& ref){
  std::swap(eType, ref.eType);
  std::swap(pStr , ref.pStr );
  return *this;
}

EventSequencerSim::TPvValue& EventSequencerSim::TPvValue::operator=(TPvValue ref) {
  return this->swap(ref);
}

EventSequencerSim::TPvValue::~TPvValue() {
  switch(eType)
  {
  case TYPE_INT:
    break;
  case TYPE_STRING:
    delete pStr;
    pStr = NULL;
    break;
  case TYPE_INTARRAY:
    delete pVecInt;
    pVecInt = NULL;
    break;
  };
}

string EventSequencerSim::TPvValue::to_string() {
  stringstream sstream;
  switch(eType)
  {
    case TYPE_INT:
      sstream << iVal;
      return sstream.str();
    case TYPE_STRING:
      return *pStr;
    case TYPE_INTARRAY:
      for (unsigned int i=0; i<pVecInt->size(); ++i){
        sstream << (*pVecInt)[i];
        if (i != pVecInt->size()-1)
          sstream << " ";
      }
      return sstream.str();
  };

  return "Type Error";
}

EventSequencerSim::TEventSequencer::TEventSequencer() : iSeqId(-1), uCurStepFid(-1),
  iPlayStatus(0), iCurStep(0), iPlayCount(0), iTotalPlayCount(0),
  iErrType(2), iErrIndex(0), bSeqStatusUpdated(true), bPvUpdated(true)
{
}

// iStart: 0 stop seqeuence  1 start (may repeat)   2 finish current round and goto next round
int EventSequencerSim::TEventSequencer::play(int iStart, unsigned fiducial) {
  ////!!!debug
  //printf("  Seq %d play(start %d, fiducial 0x%x)\n", iSeqId, iStart, fiducial);

  if (iStart == 0) {
    iPlayStatus       = 0;
    bSeqStatusUpdated = true;
    return 0;
  }

  if (iErrType != 2) // Valid Sequence
    return 0;

  if (iStart == 1)
    fiducial += 3; // min delay for PV thread/evgr thread communication

  iPlayStatus = 1;

  static const float lfSyncMarker[] = { 0.5f, 1, 5, 10, 30, 60, 120, 360 };
  if (iSyncMarker < 0 || iSyncMarker > 7 )
    iSyncMarker = 0;

  const int syncStep  = (int) (360/lfSyncMarker[iSyncMarker]);
  int delStart = syncStep - (fiducial % syncStep);
  if (delStart == syncStep) delStart = 0;

  iCurStep      = 0;
  int delStep0  = delStart + computeDelay(fiducial + delStart, iCurStep);
  if (delStep0 == 0) delStep0 = syncStep;

  //!!!debug
  //printf("  Seq %d syncStep %03d delStart 0%03d delStep0 %03d\n", iSeqId, syncStep, delStart, delStep0);

  uCurStepFid = (fiducial + delStep0) % Pds::TimeStamp::MaxFiducials;
  iPlayStatus = 1; // Waiting

  if (iStart == 1)
    iPlayCount  = 0;

  //!!!debug
  //printf("    Seq %d cur Step %03d stepFiducial 0x%x\n", iSeqId, iCurStep, uCurStepFid);

  bSeqStatusUpdated = true;
  return 0;
}

int EventSequencerSim::TEventSequencer::loadSeq(int len,
    std::vector<int>& vA, std::vector<int>& vB, std::vector<int>& vC, std::vector<int>& vD) {
  play(0,0);

  //!!!debug
  printf("  Seq %d loadSeq()\n", iSeqId);

  if (vA.size() < (size_t)len) vA.resize(len);
  if (vB.size() < (size_t)len) vB.resize(len);
  if (vC.size() < (size_t)len) vC.resize(len);
  if (vD.size() < (size_t)len) vD.resize(len);

  eventSeq.resize(len);
  for (int i=0; i<len; ++i) {
    TEventSequencer::TEventStep& step = eventSeq[i];
    step.event = vA[i];
    step.dbeam = vB[i];
    step.dfidu = vC[i];
    step.burst = vD[i];

    //!!!debug
    printf("  Seq %d step %03d  code %03d dbeam %03d dfidu %03d burst %03d\n",
           iSeqId, i, step.event, step.dbeam, step.dfidu, step.burst );
  }

  //!!!debug
  printf("  ** Seq %d len %03d\n", iSeqId, (int) eventSeq.size());

  iPlayCount      = 0;
  iTotalPlayCount = 0;

  /*
     Error Type:
       0: Internal Error  1: Unknown Error  2: Valid Sequence
       3: Inv Event Code  4: Inv Beam Delay 5: Inv Fid Delay
       6: Zero Length     7: Inv Hutch ID   8: Inv EC Hutch ID
   */
  if (len == 0) {
    iErrType  = 6; // Zero Length
    iErrIndex = 0;
  }
  else {
    iErrType  = 2; // Valid Sequence
    iErrIndex = 0;
  }

  bSeqStatusUpdated = true;
  return 0;
}

int EventSequencerSim::TEventSequencer::getSeqEvent(unsigned fiducial, std::vector<int>& lEvents) {
  if (iPlayStatus == 0 || eventSeq.empty())
    return 0;

  while (uCurStepFid == fiducial) {
    //!!!debug
    //printf("  ** Fiducial 0x%x Seq %d step %03d event %03d\n",fiducial, iSeqId, iCurStep, eventSeq[iCurStep].event);

    iPlayStatus = 2;
    if (eventSeq[iCurStep].event != 0)
      lEvents.push_back( eventSeq[iCurStep].event );

    if ( iCurStep == (int) eventSeq.size()-1) {
      ++iPlayCount;
      ++iTotalPlayCount;

      ////!!!debug
      //printf("  Seq %d playCount %03d TotalCount %03d\n", iSeqId, iPlayCount, iTotalPlayCount);
      if (iPlayMode == 0 ||
          (iPlayMode == 1 && iPlayCount >= iRepeatCount) )
        return play(0, fiducial);

      return play(2, fiducial);
    }

    ++iCurStep;
    uCurStepFid = (fiducial + computeDelay(fiducial,iCurStep)) % Pds::TimeStamp::MaxFiducials;
    bSeqStatusUpdated = true;
  }

  return 0;
}

int EventSequencerSim::TEventSequencer::computeDelay(unsigned fiducial, int iStep) {
  if (eventSeq[iCurStep].dbeam == 0)
    return eventSeq[iCurStep].dfidu;

  const int beamStep = (int) (360/fBeamRate);
  const int delBeam = beamStep - (fiducial % beamStep);

  return delBeam + beamStep*(eventSeq[iCurStep].dbeam-1) + eventSeq[iCurStep].dfidu;
}
